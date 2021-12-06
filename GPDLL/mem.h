#pragma once

namespace mem
{
	void Patch(BYTE* dst, BYTE* src, unsigned int size);
	void Nop(BYTE* dst, unsigned int size);
	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);
	bool Detour(void* src, void* dst, int len);
	BYTE* Trampoline(void* src, void* dst, int len);
}