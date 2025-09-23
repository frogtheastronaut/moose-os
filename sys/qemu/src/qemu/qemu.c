#include <stdint.h>
#include <stdbool.h>

static inline void cpuid(uint32_t code, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code));
}

bool detect_qemu(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax,&ebx,&ecx,&edx);
    // ECX bit 31 = hypervisor
    if (!(ecx & (1 << 31))) return false;

    cpuid(0x40000000, &eax,&ebx,&ecx,&edx);
    // Hypervisor vendor string is usually "TCGTCGTCG" or "KVMKVMKVM"
    return (ebx || ecx || edx); // just check non-zero vendor string
}
