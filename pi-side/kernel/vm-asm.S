#include "arm-coprocessor-insts.h"

@ used to clear register before CP15 operation.
#define CLR(reg) mov reg, #0

.globl get_cache_type
get_cache_type:
    mrc p15, 0, R0, c0, c0, 1;  ; /* returns Cache Type register */
    bx lr

.globl write_domain_access_ctrl
write_domain_access_ctrl:
    DOMAIN_CTRL_WR(R0)
    CLR(R0)
    PREFETCH_FLUSH(R0)
    bx lr

.global cp15_barrier
cp15_barrier:
    CLR(R0)
    DSB(R0)
    PREFETCH_FLUSH(R0)
    bx lr

.globl set_procid_ttbr0
set_procid_ttbr0:
    CLR(R2)
    @ Following recipe on B-25
    @ Change ASID to 0
    ASID_SET(r2)
    PREFETCH_FLUSH(R2)
    @ Change Translation Table Base Register
    TTBR0_SET(r1)
    TTBR1_SET(r2)
    TTBR_BASE_CTRL_WR(r2)
    PREFETCH_FLUSH(R2)
    @ Change ASID
    ASID_SET(r0)
    @ Invalidate the BTB, B-24
    FLUSH_BTB(R2)
    DSB(R2)
    PREFETCH_FLUSH(R2)
    bx lr

.globl mmu_enable
mmu_enable:
    CLR(R2)
    DSB(R2)
    INV_ICACHE(R2)
    INV_DCACHE(R2)
    INV_TLB(R2)
    DSB(R2)
    mcr p15, 0, R0, c1, c0, 0
    FLUSH_BTB(R2)
    DSB(R2)
    PREFETCH_FLUSH(R2)
    bx lr

.globl mmu_disable
mmu_disable:
    CLR(R2)
    DSB(R2)
    INV_ICACHE(R2)
    CLEAN_DCACHE(R2)
    DSB(R2)
    mcr p15, 0, R0, c1, c0, 0
    FLUSH_BTB(R2)
    DSB(R2)
    PREFETCH_FLUSH(R2)
    bx lr

.globl mmu_sync
mmu_sync:
    CLR(R2)
    CLEAN_DCACHE_LINE(R0)
    DSB(R2)
    INV_ITLB_ENTRY(R1)
    INV_DTLB_ENTRY(R1)
    FLUSH_BTB(R2)
    DSB(R2)
    PREFETCH_FLUSH(R2)
    bx lr

.globl mmu_inv_asid
mmu_inv_asid:
    CLR(R2)
    INV_ITLB_ASID(R0)
    INV_DTLB_ASID(R0)
    FLUSH_BTB(R2)
    DSB(R2)
    PREFETCH_FLUSH(R2)
    bx lr

