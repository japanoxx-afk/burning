#include <windows.h>
#include <string.h>
#include <wchar.h>

static decltype(&ResumeThread) realResume = ResumeThread;
static decltype(&CreateProcessA) realCreate = CreateProcessA;
static decltype(&RegQueryValueExA) realQuery = RegQueryValueExA;
static DWORD gamePid;
static HANDLE gameProcess;

static bool Inject(HANDLE process, const wchar_t* path) {
    SIZE_T size = (wcslen(path)+1)*sizeof(wchar_t), written;
    void* remote = VirtualAllocEx(process, NULL, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!remote) return false;
    if (!WriteProcessMemory(process, remote, path, size, &written) || written != size) return false;
    HANDLE thread = CreateRemoteThread(process,NULL,0,(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"),remote,0,NULL);
    if (!thread) return false;
    DWORD result = 0;
    if (WaitForSingleObject(thread,20000)==WAIT_OBJECT_0) GetExitCodeThread(thread,&result);
    CloseHandle(thread);
    if (result) VirtualFreeEx(process,remote,0,MEM_RELEASE);
    return result != 0;
}
static DWORD WINAPI BeforeResume(HANDLE thread) {
    if (gamePid && GetProcessIdOfThread(thread)==gamePid) {
        wchar_t dll[32768];
        if (!GetEnvironmentVariableW(L"BG_DISPLAY_DLL",dll,32768) || (wcscmp(dll,L"original") && !Inject(gameProcess,dll))) {
            TerminateProcess(gameProcess,72);
            MessageBoxW(NULL,L"고해상도 DLL을 불러오지 못했습니다.",L"Burning Ground",MB_ICONERROR);
            SetLastError(ERROR_DLL_INIT_FAILED);
            return (DWORD)-1;
        }
        gamePid = 0;
        CloseHandle(gameProcess);
    }
    return realResume(thread);
}
static BOOL WINAPI StartGame(LPCSTR app,LPSTR cmd,LPSECURITY_ATTRIBUTES pa,LPSECURITY_ATTRIBUTES ta,BOOL inherit,DWORD flags,LPVOID env,LPCSTR dir,LPSTARTUPINFOA si,LPPROCESS_INFORMATION pi) {
    wchar_t root[32768];
    GetEnvironmentVariableW(L"BG_GAME_DIR",root,32768);
    wchar_t path[32768];
    wsprintfW(path,L"%s\\StarCraft.exe",root);
    wchar_t command[32768];
    wsprintfW(command,L"\"%s\"",path);
    STARTUPINFOW sw = {}; sw.cb=sizeof(sw); sw.dwFlags=si->dwFlags; sw.wShowWindow=si->wShowWindow;
    BOOL ok = CreateProcessW(path,command,pa,ta,inherit,flags,env,root,&sw,pi);
    if (ok) { gamePid=pi->dwProcessId; DuplicateHandle(GetCurrentProcess(),pi->hProcess,GetCurrentProcess(),&gameProcess,0,FALSE,DUPLICATE_SAME_ACCESS); }
    return ok;
}
static LSTATUS WINAPI Query(HKEY key,LPCSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD size) {
    if (name && (!lstrcmpiA(name,"Program") || !lstrcmpiA(name,"InstallPath"))) {
        char root[MAX_PATH], value[MAX_PATH];
        GetEnvironmentVariableA("BG_GAME_DIR",root,MAX_PATH);
        lstrcpyA(value,root);
        if (!lstrcmpiA(name,"Program")) lstrcatA(value,"\\StarCraft.exe");
        DWORD need=lstrlenA(value)+1;
        if(type) *type=REG_SZ;
        if(!size) return ERROR_INVALID_PARAMETER;
        if(!data){*size=need;return ERROR_SUCCESS;}
        if(*size<need){*size=need;return ERROR_MORE_DATA;}
        memcpy(data,value,need);*size=need;return ERROR_SUCCESS;
    }
    return realQuery(key,name,reserved,type,data,size);
}
BOOL WINAPI DllMain(HINSTANCE,DWORD reason,LPVOID) {
    if(reason!=DLL_PROCESS_ATTACH) return TRUE;
    BYTE* base=(BYTE*)GetModuleHandleW(NULL);
    auto nt=(IMAGE_NT_HEADERS*)(base+((IMAGE_DOS_HEADER*)base)->e_lfanew);
    auto desc=(IMAGE_IMPORT_DESCRIPTOR*)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(;desc->Name;desc++) {
        auto names=(IMAGE_THUNK_DATA*)(base+desc->OriginalFirstThunk);
        auto slots=(IMAGE_THUNK_DATA*)(base+desc->FirstThunk);
        for(;names->u1.AddressOfData;names++,slots++) {
            if(IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            char* name=(char*)((IMAGE_IMPORT_BY_NAME*)(base+names->u1.AddressOfData))->Name;
            void* replacement=NULL;
            if(!strcmp(name,"ResumeThread")) replacement=(void*)&BeforeResume;
            if(!strcmp(name,"CreateProcessA")) replacement=(void*)&StartGame;
            if(!strcmp(name,"RegQueryValueExA")) replacement=(void*)&Query;
            if(replacement) { DWORD old; VirtualProtect(&slots->u1.Function,4,PAGE_READWRITE,&old); slots->u1.Function=(DWORD)replacement; VirtualProtect(&slots->u1.Function,4,old,&old); }
        }
    }
    return TRUE;
}
