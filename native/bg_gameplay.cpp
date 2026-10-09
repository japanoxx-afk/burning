#include <windows.h>
#include <string.h>
#include "bg_gameplay.h"
#include "bg_gameplay_policy.h"
#include "sc_engine.h"
#include "sc_addresses.h"
#include "sc_console.h"
#include "sc_hook.h"
#include "sc_log.h"
#include "sc_unit.h"
#include "sc_session.h"
#include "sc_circles.h"
#include "sc_env.h"

static ScHook queueHook, windowHook, rallyHook;
extern "C" { DWORD speedNext, selectNext, rallyNext, checkPosition; }
static WNDPROC oldProc;
static bool internal; static unsigned epoch;
struct Member { DWORD unit; BYTE unique,player; };
static Member group[1700]; static int groupN,visibleN,next;
static BYTE pending[32];static unsigned pendingN,lastFrame;
static volatile LONG request;
static bool Alive(const Member&m) {
 return ScUnitPtrValid(m.unit)&&ScUnitUniqueness(m.unit)==m.unique&&ScUnitPlayer(m.unit)==m.player&&ScUnitHitPoints(m.unit)&&ScUnitSprite(m.unit)&&ScUnitInOwnPlayerList(m.unit);
}
static bool SinglePlayer(){return *(DWORD*)ScRuntimeAddr(0x0059688Cu)==0;}
static bool EmitSelect(const Member*members,int n) {
 BYTE buf[50]={9,0};
 for(int i=0;i<n;i++)if(Alive(members[i])){WORD tag=ScUnitTag(members[i].unit);memcpy(buf+2+buf[1]*2,&tag,2);++buf[1];}
 if(buf[1])((void(__attribute__((fastcall))*)(const void*,unsigned))queueHook.trampoline)(buf,2+buf[1]*2);
 return buf[1]!=0;
}
extern "C" void SC_GAME_ENTRY BgSelectionChanged(){if(!internal){groupN=visibleN=0;pendingN=0;}}
extern "C" void __attribute__((naked)) BgSelectGate(){
 __asm__ __volatile__("pushfl\n\tpushal\n\tcall _BgSelectionChanged\n\tpopal\n\tpopfl\n\tpush %ebp\n\tjmp *_selectNext");
}
extern "C" DWORD SC_GAME_ENTRY BgSpeedBase(DWORD unit,DWORD base){
 // BG's extended iscript: Crusader walks 5 pixels per wait(1),
 // Chrono Battlesuit walks 4 per wait(1). Both use iscript movement (type 2).
 // Preserve each animation and all stock status modifiers; scale movement itself.
 if(ScUnitPtrValid(unit)&&*(WORD*)(unit+0x64)==61)return BgCrusaderSpeed(base);
 return base;
}
extern "C" void __attribute__((naked)) BgSpeedGate(){
 __asm__ __volatile__("pushfl\n\tpushal\n\tpush 28(%esp)\n\tpush 24(%esp)\n\tcall _BgSpeedBase\n\tadd $8,%esp\n\tmov %eax,28(%esp)\n\tpopal\n\tpopfl\n\tjmp *_speedNext");
}
struct Point { WORD x,y; };
static void SetPosition(DWORD unit,Point p){
 DWORD a=p.x,c=p.y,d=unit,f=ScRuntimeVa(0x004EB9F0u);
 __asm__ __volatile__("call *%3":"+a"(a),"+c"(c),"+d"(d):"r"(f):"memory","cc");
}
static bool CheckPosition(DWORD unit,const Point*in,Point*out){
 DWORD a=0;
 __asm__ __volatile__("push $0\n\tpush $1\n\tpush %3\n\tpush %2\n\tpush %1\n\tcall *_checkPosition"
  :"+a"(a):"r"(unit),"r"(in),"r"(out):"ecx","edx","memory","cc");
 return a!=0;
}
extern "C" void SC_GAME_ENTRY BgRallySpawn(DWORD unit,DWORD factory){
 if(!SinglePlayer()||!ScUnitPtrValid(unit)||!ScUnitPtrValid(factory))return;
 DWORD target=*(DWORD*)(factory+0xfc);Point wanted=*(Point*)(factory+0xf8);
 if(target==factory||!wanted.x)return; // No explicit rally was set.
 WORD w=*(WORD*)ScRuntimeAddr(0x0057F1D4u),h=*(WORD*)ScRuntimeAddr(0x0057F1D6u);
 if(wanted.x>=w*32u||wanted.y>=h*32u)return;
 Point prev=*(Point*)(unit+0x28),actual;
 SetPosition(unit,wanted);
 if(!CheckPosition(unit,&wanted,&actual)){SetPosition(unit,prev);ScLog("BG RALLY blocked: kept normal production position");return;}
 DWORD d=unit,f=ScRuntimeVa(0x00493CA0u);
 __asm__ __volatile__("push $0\n\tcall *%1":"+D"(d):"r"(f):"eax","ecx","edx","memory","cc");
 SetPosition(unit,actual);
 DWORD a=unit;f=ScRuntimeVa(0x00494160u);
 __asm__ __volatile__("call *%1":"+a"(a):"r"(f):"ecx","edx","memory","cc");
 ScLog("BG RALLY spawn unit=%u position=%u,%u target=%u,%u",*(WORD*)(unit+0x64),actual.x,actual.y,wanted.x,wanted.y);
}
extern "C" void __attribute__((naked)) BgRallyGate(){
 __asm__ __volatile__("pushfl\n\tpushal\n\tpush 24(%esp)\n\tpush 32(%esp)\n\tcall _BgRallySpawn\n\tadd $8,%esp\n\tpopal\n\tpopfl\n\tjmp *_rallyNext");
}
static void __attribute__((fastcall)) SC_GAME_ENTRY Queue(const BYTE*buf,unsigned n){
 // Orders only: selection, chat, production, spells not listed here retain BG behavior.
 BYTE op=n?buf[0]:0;
 bool order=(op==0x14&&n==10)||(op==0x15&&n==11)||(op==0x1a&&n==2)||(op==0x2b&&n==2);
 if(!internal&&SinglePlayer()&&groupN>24&&order){memcpy(pending,buf,n);pendingN=n;next=0;ScLog("BG GROUP order=%02x count=%d",op,groupN);return;}
 ((void(__attribute__((fastcall))*)(const void*,unsigned))queueHook.trampoline)(buf,n);
}
static bool EditOpen(){
 DWORD d=*(DWORD*)ScRuntimeAddr(SC_VA_DIALOG_LIST);
 for(int roots=0;d&&roots<64;roots++){
  if(!ScReadable(d,SC_BINDLG_SIZE))return true;
  DWORD c=ScDlgChild(d);
  for(int nodes=0;c&&nodes<128;nodes++){
   if(!ScReadable(c,SC_BINDLG_SIZE))return true;
   if(*(WORD*)(c+SC_BINDLG_OFF_TYPE)==8&&(*(DWORD*)(c+SC_BINDLG_OFF_FLAGS)&8))return true;
   c=*(DWORD*)(c+SC_BINDLG_OFF_NEXT);
  }
  d=*(DWORD*)(d+SC_BINDLG_OFF_NEXT);
 }
 return false;
}
static LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
 // F2 is translated to Local.dll's accelerator command before WM_KEYDOWN.
 if(msg==WM_COMMAND&&LOWORD(w)==0x9c7f&&ScConsoleInGame()==1&&SinglePlayer()&&!EditOpen()){
  InterlockedExchange(&request,2);return 0;
 }
 bool key=w==VK_F2||w==VK_OEM_PERIOD;
 if(msg==WM_KEYDOWN&&key&&ScConsoleInGame()==1&&SinglePlayer()&&!EditOpen()){
  if(!(l&(1L<<30)))InterlockedExchange(&request,w==VK_F2?2:1);
  return 0;
 }
 if(msg==WM_CHAR&&w=='.'&&ScConsoleInGame()==1&&SinglePlayer()&&!EditOpen())return 0;
 return CallWindowProcW(oldProc,hwnd,msg,w,l);
}
static void SelectAll(int kind){
 DWORD player=*(DWORD*)ScRuntimeAddr(SC_VA_PLAYER_ID_512688);if(player>=8)return;
 DWORD u=*(DWORD*)(ScRuntimeVa(SC_VA_PLAYER_UNIT_LIST)+player*4);groupN=0;pendingN=0;
 for(int walk=0;u&&walk<1700;walk++){
  if(!ScUnitPtrValid(u))break;
  WORD id=*(WORD*)(u+0x64);DWORD status=*(DWORD*)(u+0xdc);
  BYTE order=*(BYTE*)(u+0x4d);
  DWORD flags=id<228?*(DWORD*)(ScRuntimeVa(0x00664080u)+id*4):0;
  Member m={u,ScUnitUniqueness(u),(BYTE)player};
  if(BgSelectable(id,flags,status,order,kind==1)&&Alive(m))group[groupN++]=m;
  u=*(DWORD*)(u+SC_CUNIT_OFF_LIST_NEXT);
 }
 if(!groupN){ScLog("BG SELECT kind=%d count=0",kind);ScCirclesHide();return;}
 visibleN=groupN<24?groupN:24;DWORD visible[24];for(int i=0;i<visibleN;i++)visible[i]=group[i].unit;
 DWORD create=ScRuntimeVa(SC_VA_CREATE_NEW_UNIT_SELECTIONS);
 internal=true;
 DWORD visiblePtr=(DWORD)visible;
 __asm__ __volatile__("push %1\n\tcall *%2" : "+a"(visiblePtr) : "r"(visibleN),"r"(create) : "ecx","edx","memory","cc");
 ((void(__attribute__((stdcall))*)(unsigned,DWORD*))ScRuntimeAddr(SC_VA_CMDACT_SELECT))(visibleN,visible);
 internal=false;
 ScCircleUnit extra[1700];int n=0;for(int i=visibleN;i<groupN;i++)extra[n++]={group[i].unit,ScUnitSprite(group[i].unit),group[i].unique,group[i].player};ScCirclesShow(extra,n);
 ScLog("BG SELECT kind=%d count=%d visible=%d",kind,groupN,visibleN);
}
void BgGameplayFrame(){
 if(epoch!=ScSessionEpoch()){epoch=ScSessionEpoch();groupN=visibleN=pendingN=0;request=0;}
 if(ScConsoleInGame()!=1||!SinglePlayer())return;
 int key=InterlockedExchange(&request,0);if(key)SelectAll(key);
 unsigned frame=*(DWORD*)ScRuntimeAddr(0x0057F23Cu);if(frame==lastFrame)return;lastFrame=frame;
 if(pendingN&&next<groupN){
  unsigned used=*(DWORD*)ScRuntimeAddr(SC_VA_BYTES_IN_CMD_QUEUE),max=*(DWORD*)ScRuntimeAddr(SC_VA_MAX_CMD_QUEUE_BYTES);
  if(used+110+pendingN>max)return;
  int n=groupN-next;if(n>24)n=24;internal=true;
  if(EmitSelect(group+next,n))
   ((void(__attribute__((fastcall))*)(const void*,unsigned))queueHook.trampoline)(pending,pendingN);
  EmitSelect(group,visibleN);internal=false;next+=n;if(next>=groupN){pendingN=0;ScLog("BG GROUP order complete count=%d",groupN);}
 }
}
static bool Patch(BYTE*p,const BYTE*expected,const BYTE*value,int n){
 if(!ScReadableAt(p,n)||memcmp(p,expected,n))return false;DWORD old;if(!VirtualProtect(p,n,PAGE_EXECUTE_READWRITE,&old))return false;
 memcpy(p,value,n);VirtualProtect(p,n,old,&old);FlushInstructionCache(GetCurrentProcess(),p,n);return true;
}
static BYTE*Rel(BYTE*p){return p+5+*(LONG*)(p+1);}
bool BgGameplayInstall(){
 BYTE*transfer=(BYTE*)ScRuntimeAddr(0x004696D0u);BYTE*speed=(BYTE*)ScRuntimeAddr(0x0047B5F0u);BYTE*sel=(BYTE*)ScRuntimeAddr(SC_VA_CMDACT_SELECT);
 if(transfer[0]!=0xe9||speed[0]!=0xe9||sel[0]!=0x55||sel[1]!=0xe9)return false;
 BYTE*wrapper=Rel(transfer);if(wrapper[14]!=0xe8)return false;BYTE*body=Rel(wrapper+14);
 if(body[0x82]!=0xe8)return false;BYTE*harvest=Rel(body+0x82);
 BYTE a[]={0x83,0xf8,5},b[]={0x83,0xf8,10};if(!Patch(harvest+0x37,a,b,3))return false;
 BYTE c[]={0x83,0xc0,0xfb},d[]={0x83,0xc0,0xf6};if(!Patch(harvest+0x3c,c,d,3))return false;
 BYTE e[]={0xb0,5},f[]={0xb0,10};if(!Patch(harvest+0x4d,e,f,2))return false;
 BYTE h[]={0x80,0xfa,5},j[]={0x80,0xfa,10};if(!Patch(body+0xa3,h,j,3)||!Patch(body+0xb3,h,j,3))return false;
 speedNext=(DWORD)Rel(speed);BYTE*sbody=Rel((BYTE*)speedNext+14);
 BYTE bonus[]={0x75,0x0d},skip[]={0xeb,0x0d};if(!Patch(sbody+0x17,bonus,skip,2))return false;
 BYTE jump[5]={0xe9};*(LONG*)(jump+1)=(BYTE*)BgSpeedGate-speed-5;BYTE oldJump[5];memcpy(oldJump,speed,5);if(!Patch(speed,oldJump,jump,5))return false;
 selectNext=(DWORD)Rel(sel+1);BYTE sj[6]={0xe9,0,0,0,0,0x90};*(LONG*)(sj+1)=(BYTE*)BgSelectGate-sel-5;BYTE so[6];memcpy(so,sel,6);if(!Patch(sel,so,sj,6))return false;
 BYTE q[]={0x55,0x8b,0xec,0x51,0xa1,0xa0,0x4a,0x65,0x00};
 if(!ScHookInstall(&queueHook,"BG command fanout",ScRuntimeAddr(SC_VA_QUEUE_COMMAND),(void*)Queue,9,q,9))return false;
 BYTE wp[]={0x55,0x8b,0xec,0x83,0xec,0x58};
 if(!ScHookInstall(&windowHook,"BG selection keys",ScRuntimeAddr(0x004D1D70u),(void*)WindowProc,6,wp,6))return false;
 oldProc=(WNDPROC)windowHook.trampoline;
 if(ScEnvOptIn("BG_RALLY_SPAWN")){
  checkPosition=ScRuntimeVa(0x0049D3E0u);
  BYTE rp[]={0x85,0xc9,0x56,0x8b,0xf0};
  if(!ScHookInstall(&rallyHook,"BG rally spawn",ScRuntimeAddr(0x00466F50u),(void*)BgRallyGate,5,rp,5))return false;
  rallyNext=(DWORD)rallyHook.trampoline;
 }
 ScCirclesInit(ScEngineModuleBase(),true);if(ScCirclesInstall()!=1)return false;
 ScLog("BG GAMEPLAY ready: mineral 10, idle '.', army F2, Crusader speed=Chrono");return true;
}
