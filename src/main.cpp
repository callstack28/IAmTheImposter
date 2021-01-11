#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;
HWND window = NULL;

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    DWORD wndProcID;
    GetWindowThreadProcessId(handle, &wndProcID);

    if (GetCurrentProcessId() != wndProcID)
    {
        return TRUE;
    }

    window = handle;
    return FALSE;
}

HWND GetProcessWindow()
{
    EnumWindows(EnumWindowsCallback, NULL);
    return window;
}

bool Detour(void* hookAddr, void* func, int numBytes)
{
    if (numBytes < 5) {
        MessageBoxA(NULL, "5 bytes were not detected!", "Error", NULL);
        return false;
    }

    DWORD curProtection;
    VirtualProtect(hookAddr, numBytes, PAGE_EXECUTE_READWRITE, &curProtection);

    memset(hookAddr, 0x90, numBytes);

    DWORD diff((DWORD)func - (DWORD)hookAddr);
    DWORD relaAddr = diff - 5;

    *(BYTE*)hookAddr = 0xE9;
    *(DWORD*)((DWORD)hookAddr + 1) = relaAddr;

    DWORD temp;
    VirtualProtect(hookAddr, numBytes, curProtection, &temp);

    return true;
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
    AllocConsole();
    FILE *f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    cout << "Is this thing on?" << endl;


    DWORD moduleBaseAddr = (DWORD)GetModuleHandleA("GameAssembly.dll");
    moduleBaseAddr = (uintptr_t)GetModuleHandle(NULL);
    DWORD hookAddr = moduleBaseAddr + 0x88C47A;

    int numBytes = 5;

    jumpBackAddr = hookAddr + numBytes;

    if (Detour((void*)hookAddr, func, numBytes))
        MessageBoxA(NULL, "hooked!", "Success!", NULL);
    else
        MessageBoxA(NULL, "hook failed!", "Failed!", NULL);


    while (true) {
        if (GetAsyncKeyState(VK_ESCAPE)) break;
        FreeLibraryAndExitThread((HMODULE)param, NULL);
        Sleep(100);
    }

    MessageBoxA(NULL, "See you next time!", "Exitting!", NULL);
    FreeLibraryAndExitThread((HMODULE)param, NULL);
    fclose(f);
    FreeConsole();
    return 0;
}




BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH)
    {
        // quick process check
        if (GetModuleHandleA("Among Us.exe") == nullptr)
        {
            MessageBoxA(nullptr, ("this cannot be injected in another process\nopen <Among Us.exe> to inject."), MB_OK, MB_OKCANCEL);
            return false;
        }

        // disable DLL_THREAD_ATTACH_DETACH reasons to call
        DisableThreadLibraryCalls(hModule);

        CreateThread(NULL, 0, &gthread, 0, 0, 0);
    }
}
