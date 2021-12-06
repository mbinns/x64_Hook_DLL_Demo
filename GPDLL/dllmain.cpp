// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>

//Local
#include "mem.h"

//Function Pointer Typedef, we learned that it was a fastcall from disassmbling it
typedef int(__fastcall* t_Subtract)(int x);

//Creating the pointer for the original function
t_Subtract o_Subtract;

//My hooked function
int __fastcall h_Subtract(int x)
{
    //Since the original function has a return value we need to return a call to the original function
    //Or just return a modified value, really depends on what you want to do.
    //If there was no return value, you can just call the function with out the return
    std::cout << "Hooked Result: " << std::dec << x+2 << std::endl;
    return o_Subtract(x+4);
}

extern "C" __declspec(dllexport)
DWORD WINAPI HackThread(HMODULE hModule)
{
    //Make File pointer for the new console you just made
    //CONOUT gets current console output for the process
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "Injected!" << std::endl;

    //Get the dynamic module base address
    uintptr_t module_base = (uintptr_t)GetModuleHandle(L"HookDemo.exe");
    
    //Relative Offset to the subtract Symbol from the module base address
    std::vector<unsigned int> offset = {0x1050};

    //Do the pointer math for the dynamic address based off the offset in the main module
    uintptr_t subtract_addr = mem::FindDMAAddy(module_base, offset);

    std::cout << "ModuleBase: " << std::hex << module_base << std::endl;
    std::cout << "SubtractAddr: " << std::hex << subtract_addr << std::endl;

    //Pointer to the original function
    o_Subtract = (t_Subtract)subtract_addr;

    //Call Trampoline which is a wrapper around the detour function, this function will set up the jumpback 
    //And ensure all of the stolen bytes are executed.
    o_Subtract = (t_Subtract)mem::Trampoline((void*)subtract_addr, (void*)h_Subtract, 16);

    while (true)
    {
        if (GetAsyncKeyState(VK_DELETE) == 1)
        {
            break;
        }
        Sleep(100);
    }

    if (f)
    {
        fclose(f);
    }
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

extern "C" __declspec(dllexport)
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

