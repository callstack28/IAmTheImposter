#include "pch.h"
#include <Windows.h>

bool Detour(void* hookAddr, void* func, int numBytes)
{
    if (numBytes < 5)
    {
        MessageBoxA(NULL, "5 bytes were not detected!", "Error", NULL);
        return 1;
    }
    else
    {
        DWORD protAddr;

        VirtualProtect(hookAddr, numBytes, PAGE_EXECUTE_READWRITE, &protAddr);

        memset(hookAddr, 0x90, numBytes);

        DWORD diff((DWORD)func - (DWORD)hookAddr);
        DWORD relaAddr = diff - 5;

        *(BYTE*)hookAddr = 0xE9;
        *(DWORD*)((DWORD)hookAddr + 1) = relaAddr;

        DWORD junk;

        VirtualProtect(hookAddr, numBytes, protAddr, &junk);

        return true;
    }
}

DWORD jumpBackAddr;

void __declspec(naked) func()
{
    __asm
    {
        xor eax, eax

        mov[esi + 0x44], eax

        jmp[jumpBackAddr]
    }
}





DWORD WINAPI gthread(LPVOID param)
{
    DWORD moduleBaseAddr = (DWORD)GetModuleHandleA("GameAssembly.dll");
    DWORD hookAddr = moduleBaseAddr + 0x88C47A;

    int numBytes = 5;

    jumpBackAddr = hookAddr + numBytes;

    Detour((void*)hookAddr, func, numBytes);

    return 0;
}




BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CreateThread(NULL, NULL, gthread, hModule, NULL, NULL);
    }
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

