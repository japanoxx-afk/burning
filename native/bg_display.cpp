#include <windows.h>
#include "sc_engine.h"
#include "sc_screen.h"
#include "sc_console.h"
#include "sc_menu.h"
#include "sc_stormpresent.h"
#include "sc_log.h"
#include "sc_session.h"
#include "bg_gameplay.h"

static BYTE* entry;
static BYTE saved[7];

extern "C" void __attribute__((force_align_arg_pointer)) InitializeDisplay() {
    DWORD old;
    VirtualProtect(entry, 7, PAGE_EXECUTE_READWRITE, &old);
    memcpy(entry, saved, 7);
    VirtualProtect(entry, 7, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, 7);
    ScLogOpen();
    BYTE* base = (BYTE*)GetModuleHandleW(NULL);
    ScEngineSetModuleBase(base);
    if (!ScEnvOptIn("SCPLUGIN_WIDESCREEN")) {
        if (ScSessionInstall(base, true) != 2) ExitProcess(73);
        ScConsoleInstall(base, true, false);
        if (!BgGameplayInstall()) ExitProcess(74);
        ScLog("BG GAMEPLAY READY original 640x480");
        return;
    }
    ScScreenInstall(base, SC_MODE_LOGONLY);
    if (!ScScreenActive()) {
        ScLog("BG DISPLAY REFUSED: mod signatures differ; terminating before video initialization");
        MessageBoxW(NULL, L"BG 모드와 해상도 패치가 충돌합니다. 로그를 확인하세요.", L"Burning Ground", MB_ICONERROR);
        ExitProcess(71);
    }
    ScMenuInstall(true);
    if (ScSessionInstall(base, true) != 2) ExitProcess(73);
    ScConsoleInstall(base, true, false);
    ScStormPresentInstall(base, true);
    ScScreenLogStats();
    if (!BgGameplayInstall()) { ScLog("BG GAMEPLAY signature conflict"); ExitProcess(74); }
    ScLog("BG DISPLAY READY %dx%d", ScScreenTargetWidth(), ScScreenTargetHeight());
}

// The MPQDraft stub runs before the original entry point; wait for its mod patches.
extern "C" void __attribute__((naked)) EntryGate() {
    __asm__ __volatile__("pushfl\n\tpushal\n\tcall _InitializeDisplay\n\tpopal\n\tpopfl\n\tjmp *%0" : : "m"(entry));
}
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    BYTE* base = (BYTE*)GetModuleHandleW(NULL);
    auto nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
    entry = base + nt->OptionalHeader.AddressOfEntryPoint;
    const BYTE expected[] = {0x6a,0x60,0x68,0xf0,0xe5,0x4f,0x00};
    if (memcmp(entry, expected, 7)) return FALSE;
    memcpy(saved, entry, 7);
    DWORD old;
    if (!VirtualProtect(entry, 7, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    entry[0] = 0xe9;
    *(DWORD*)(entry+1) = (DWORD)((BYTE*)&EntryGate - entry - 5);
    entry[5] = entry[6] = 0x90;
    VirtualProtect(entry, 7, old, &old);
    FlushInstructionCache(GetCurrentProcess(), entry, 7);
    return TRUE;
}
