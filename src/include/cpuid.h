/* equivelent to gcc __get_cpuid */
void getCpuID(int level, int *eax, int *ebx, int *ecx, int *edx);

void getCpuID(int level, int *eax, int *ebx, int *ecx, int *edx)
{
    int a, b, c, d;
    __asm__ __volatile__ (
        "cpuid"
        : "=a" (a), "=b" (b), "=c" (c), "=d" (d)
        : "a" (level)
    );
    *eax = a;
    *ebx = b;
    *ecx = c;
    *edx = d;
}