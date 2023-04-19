#ifndef STDBOOL_INCLUDE
#include <stdbool.h>
#define STDBOOL_INCLUDE
#endif
#ifndef CPUID_INCLUDE
#include "cpuid.h"
#define CPUID_INCLUDE
#endif

const uint32_t CPUID_FLAG_MSR = 1 << 5;
 
bool cpuHasMSR()
{
   static uint32_t a, unused, d; // eax, unused, edx
   getCpuID(1, &a, &unused, &unused, &d);
   return d & CPUID_FLAG_MSR;
}
 
void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
   asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
   asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}