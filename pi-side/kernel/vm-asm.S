@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

@ used to clear register before CP15 operation.
#define CLR(reg) mov reg, #0 

@ used to be called "drain write buffer"
@ includes all cache operations.  is a superset of
@ > DMB
#define DSB(Rd)             mcr p15, 0, Rd, c7, c10, 4

#define DMB(Rd)             mcr p15, 0, Rd, c7, c10, 5

@ must flush the prefetch buffer whenever you change a virtual
@ mapping (ASID, PTE, etc) since it will have stale instructions.
@
@ if you are doing this, likely have to do a DSB before to make
@ sure whatever invalidation you did has completed.
#define ISB(Rd) PREFETCH_FLUSH(Rd)
#define PREFETCH_FLUSH(Rd)  mcr p15, 0, Rd, c7, c5, 4

@ must do this after changing any MMU stuff, ASID, etc.
#define FLUSH_BTB(Rd)         mcr p15, 0, Rd, c7, c5, 6

@ need to figure out if have a unified or separate I/D cache/TLB

@ Work-around for bug in ARMv6 if we have seperate I/D.  Taken from:
@   https://elixir.bootlin.com/linux/latest/source/arch/arm/mm/cache-v6.S
@ MUST HAVE INTERRUPTS DISABLED!
@ XXX: patch feedback implies we need this for other operations too?
#define INV_ICACHE(Rd)                                           \
    mov Rd, #0                  ;                                   \
    mcr p15, 0, Rd, c7, c5, 0   ; /* invalidate entire I-cache */   \
    mcr p15, 0, Rd, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, Rd, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    mcr p15, 0, Rd, c7, c5, 0;  ; /* invalidate entire I-cache */   \
    .rept   11                  ; /* ARM Ltd recommends at least 11 nops */\
    nop                         ;                                   \
    .endr

#define INV_DCACHE(Rd)      mcr p15, 0, Rd, c7, c6, 0
#define INV_ALL_CACHES(Rd)  mcr p15, 0, Rd, c7, c7, 0
#define CLEAN_DCACHE(Rd)    mcr p15, 0, Rd, c7, c10, 0
#define CLEAN_DCACHE_LINE(Rd)    mcr p15, 0, Rd, c7, c10, 1

@ invalidate TLB entries.
#define INV_ITLB(Rd)        mcr p15, 0, Rd, c8, c5, 0
#define INV_ITLB_ENTRY(Rd)  mcr p15, 0, Rd, c8, c5, 1
#define INV_ITLB_ASID(Rd)   mcr p15, 0, Rd, c8, c5, 2
#define INV_DTLB(Rd)        mcr p15, 0, Rd, c8, c6, 0
#define INV_DTLB_ENTRY(Rd)  mcr p15, 0, Rd, c8, c6, 1
#define INV_DTLB_ASID(Rd)   mcr p15, 0, Rd, c8, c6, 2
@ invalidate unified TLB or both I/D TLB
#define INV_TLB(Rd)         mcr p15, 0, Rd, c8, c7, 0

#define SET_DAC(Rd)  mcr p15, 0, Rd, c3, c0, 0
#define SET_ASID(Rd)  mcr p15, 0, Rd, c13, c0, 1
#define SET_TTBR0(Rd)  mcr p15, 0, Rd, c2, c0, 0
#define GET_TTBR0(Rd)  mrc p15, 0, Rd, c2, c0, 0
#define SET_TTBR1(Rd)   mcr p15, 0, Rd, c2, c0, 1
#define SET_TTBCR(Rd)   mcr p15, 0, Rd, c2, c0, 2

.globl get_cache_type
get_cache_type:
    mrc p15, 0, R0, c0, c0, 1;  ; /* returns Cache Type register */
    bx lr

.globl write_domain_access_ctrl
write_domain_access_ctrl:
    SET_DAC(R0)
    CLR(R0)
    PREFETCH_FLUSH(R0)
    bx lr

.globl set_procid_ttbr0
set_procid_ttbr0:
    CLR(R2)
    @ Following recipe on B-25
    @ Change ASID to 0
    SET_ASID(r2)
    PREFETCH_FLUSH(R2)
    @ Change Translation Table Base Register
    SET_TTBR0(r1)
    SET_TTBR1(r2)
    SET_TTBCR(r2)
    PREFETCH_FLUSH(R2)
    @ Change ASID
    SET_ASID(r0)
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

