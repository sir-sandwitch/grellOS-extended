#ifndef IO_INCLUDE
#include "io.h"
#define IO_INCLUDE
#endif
#ifndef MSR_INCLUDE
#include "msr.h"
#define MSR_INCLUDE
#endif
#ifndef STDBOOL_INCLUDE
#include <stdbool.h>
#define STDBOOL_INCLUDE
#endif
#ifndef CPUID_INCLUDE
#include "cpuid.h"
#define CPUID_INCLUDE
#endif

#define APIC_EOI_REG 0xB0
#define APIC_EOI 0x00
#define APIC_BASE_MSR 0x1B


void apic_init(void);
void apic_enable(void);
void apic_disable(void);
bool check_apic(void);

bool check_apic(void){
    unsigned int eax, unused, edx;
    getCpuID(1, &eax, &unused, &unused, &edx);
    return edx & (1 << 22);
}

void apic_disable(void){
    /* clear bit 11 of the msr to disable apic */
    uint32_t lo, hi;
    cpuGetMSR(APIC_BASE_MSR, &lo, &hi);
    lo &= ~(1 << 11);
    cpuSetMSR(APIC_BASE_MSR, lo, hi);
}