//#include "mem.h"
#include "pch.h"
#include <vector>
#include "mem.h"
#include <iostream>

void mem::Patch(BYTE* dst, BYTE* src, unsigned int size)
{
	//Keep the permissions of the old segment of memory for restoration
	DWORD oldprotect;

	//Set the segment for R/W/X and store the old protection values
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);

	//Write our patch into the memory segment
	memcpy(dst, src, size);

	//Restore old protection values
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void mem::Nop(BYTE* dst, unsigned int size)
{
	//Keep the permissions of the old segment of memory for restoration
	DWORD oldprotect;

	//Set the segment for R/W/X and store the old protection values
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);

	//NOPssssssssssss
	memset(dst, 0x90, size);

	//Restore old protection values
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

uintptr_t mem::FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets)
{
	//Base Address
	uintptr_t addr = ptr;

	//This walks the pointer chain to find the spot in dynamic memory where the address is stored
	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		//Add the offset to the address
		addr += offsets[i];
	}
	
	//Return the dynamic address of the space in memory we want
	return addr;
}

bool mem::Detour(void* src, void* dst, int len) 
{
	if (len < 12)
	{ 
		return FALSE;
	}
	
	std::cout << "Hook Address: " << std::hex << (uintptr_t) dst << std::endl;

	//Clear out 16 bytes so we dont leave any dangling functions around
	mem::Nop((BYTE*)src, len);

	//Keep the permissions of the old segment of memory for restoration
	DWORD oldprotect;
	//Set the segment for R/W/X and store the old protection values
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldprotect);

	//Mov destination address into RAX
	//Writes mov rax, <SOMETHING> into memory
	*(BYTE*)src = 0x48;
	*((BYTE*)src + 1) = 0xb8;

	//Do the offset math FIRST, then cast and dereference otherwise you'll clobber
	*(uintptr_t*)((uintptr_t)src + 2) = (uintptr_t)dst;

	//Write the Jump
	*((BYTE*)src + 10) = 0xff;
	*((BYTE*)src + 11) = 0xe0;

	//Restore old protection values
	VirtualProtect(src, len, oldprotect, &oldprotect);
	//mem::Patch((BYTE*)src, stub, sizeof(stub));
	return true;
}

BYTE* mem::Trampoline(void* src, void* dst, int len)
{
	if (len < 16)
	{
		return FALSE;
	}

	//Gateway Function
	BYTE* gateway = (BYTE*)VirtualAlloc(0, len + 20, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	std::cout << "Gateway Address: " << std::hex << (uintptr_t)gateway << std::endl;

	//Save stolen bytes for later execution, this is creating the gateway function
	//Check to see if gateway wasn't allocated
	if (gateway)
	{
		//This copies the original operations into the gateway, but we have a relative instruciton 
		//performing a mov into RCX
		//so we need to calculate the address dynamically when writing the trampoline
		memcpy_s(gateway, len, src, len);

		//MOV RCX, <SOMETHING>
		*(gateway + 9) = 0x48;
		*(gateway + 10) = 0xb9;
		
		//Calculate absolute address to move into RCX
		*(uintptr_t*)((uintptr_t)gateway + 11) = (uintptr_t)GetProcAddress(GetModuleHandle(L"msvcp140.dll"), "?cout@std@@3V?$basic_ostream@DU?$char_traits@D@std@@@1@A");


		//Sign extend 6byte address to 8byte
		*(gateway + len + 1) = 0x00;
		*(gateway + len + 2) = 0x00;

		//Mov jump back destination address into RAX
		//Writes mov rax, <SOMETHING> into memory
		*(gateway + len + 3) = 0x48;
		*(gateway + len + 4) = 0xb8;
		uintptr_t jump_back_addr = (uintptr_t)src + len;
		std::cout << "Jump Back Address: " << std::hex << jump_back_addr << std::dec << std::endl;
		*(uintptr_t*)((uintptr_t)gateway + len + 5) = jump_back_addr;

		//Write the Jump
		*((BYTE*)gateway + len + 13) = 0xff;
		*((BYTE*)gateway + len + 14) = 0xe0;

		mem::Detour(src, dst, len);
		return gateway;
	}
	return nullptr;
}
