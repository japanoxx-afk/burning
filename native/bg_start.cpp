#include <windows.h>
#include <stdio.h>
#include <wchar.h>
int wmain(int argc,wchar_t** argv) {
    if(argc!=3) return 2;
    STARTUPINFOW si={}; si.cb=sizeof(si); PROCESS_INFORMATION pi={};
    wchar_t command[32768]; swprintf(command,32768,L"\"%s\"",argv[1]);
    if(!CreateProcessW(argv[1],command,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,NULL,&si,&pi)) return 3;
    SIZE_T size=(wcslen(argv[2])+1)*2, written=0;
    void* remote=VirtualAllocEx(pi.hProcess,NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    bool ok=remote && WriteProcessMemory(pi.hProcess,remote,argv[2],size,&written) && written==size;
    HANDLE thread=ok?CreateRemoteThread(pi.hProcess,NULL,0,(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW"),remote,0,NULL):NULL;
    DWORD result=0;
    if(thread && WaitForSingleObject(thread,20000)==WAIT_OBJECT_0) GetExitCodeThread(thread,&result);
    if(thread) CloseHandle(thread);
    if(!result) {TerminateProcess(pi.hProcess,73);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return 4;}
    VirtualFreeEx(pi.hProcess,remote,0,MEM_RELEASE);
    ResumeThread(pi.hThread);
    printf("BG launcher PID=%lu\n",pi.dwProcessId);
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD code;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hProcess);
    return (int)code;
}
