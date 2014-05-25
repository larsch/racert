

#include "tick_count.h"
#include <Windows.h>

uint64_t tick_count::counter() {
	uint64_t count;
	QueryPerformanceCounter((LARGE_INTEGER*)&count);
	return count;
}
uint64_t tick_count::freq() {
	uint64_t freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	return freq;
}
