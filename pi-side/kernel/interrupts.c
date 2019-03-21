/*
 * interrupts-c.c: some interrupt support code.  Default handlers, 
 * interrupt installation.
 */
#include <libpi/rpi.h>
#include "interrupts.h"
#include "layout.h"
#include "syscall.h"

#define UNHANDLED(msg,r) \
	panic("ERROR: unhandled exception <%s> at PC=%x\n", msg,r)

void interrupt_vector(unsigned pc) {
	UNHANDLED("general interrupt", pc);
}
void fast_interrupt_vector(unsigned pc) {
	UNHANDLED("fast", pc);
}
void reset_vector(unsigned pc) {
	UNHANDLED("reset vector", pc);
}
void undefined_instruction_vector(unsigned pc) {
	UNHANDLED("undefined instruction", pc);
}
void prefetch_abort_vector(unsigned pc) {
	UNHANDLED("prefetch abort", pc);
}
void data_abort_vector(unsigned pc) {
	UNHANDLED("data abort", pc);
}

static int int_intialized_p = 0;

// wait: which was +8
enum {
    RESET_INC = -1,     // cannot return from reset
    UNDEFINED_INC = 4,  // 
    SWI_INC = 4,        // address of instruction after SWI
    PREFETCH_INC = 4,        // aborted instruction + 4
};

// call before int_init() to override.
void int_set_handler(int t, interrupt_t handler) {
    interrupt_t *src = (void*)&_interrupt_table;
    demand(t >= RESET_INT && t < FIQ_INT && t != INVALID, invalid type);
    src[t] = handler;

    demand(!int_intialized_p, must be called before copying vectors);
}

/*
 * Copy in interrupt vector table and FIQ handler _table and _table_end
 * are symbols defined in the interrupt assembly file, at the beginning
 * and end of the table and its embedded constants.
 */
static void install_handlers(void) {
    unsigned *dst = (void*)RPI_VECTOR_START,
            *src = &_interrupt_table,
            n = &_interrupt_table_end - src;
    for(int i = 0; i < n; i++)
        dst[i] = src[i];
}

void cpsr_print_mode(unsigned cpsr_r) {
    switch(cpsr_r & 0b11111) {
        case USER_MODE: printk("user mode\n"); break;
        case FIQ_MODE: printk("fiq mode\n"); break;
        case IRQ_MODE: printk("irq mode\n"); break;
        case SUPER_MODE: printk("supervisor mode\n"); break;
        case ABORT_MODE: printk("abort mode\n"); break;
        case UNDEF_MODE: printk("undef mode\n"); break;
        case SYS_MODE: printk("sys mode\n"); break;
        default: panic("invalid cpsr: %b\n", cpsr_r);
    }
}

void cp15_barrier();
unsigned cpsr_read(void);
unsigned cpsr_read_c(void) {
    return cpsr_read() & 0b11111;
}
void swi_setup_stack(unsigned stack_addr);

void int_init(void) {
    // BCM2835 manual, section 7.5: turn off all GPIO interrupts.
    PUT32(INTERRUPT_DISABLE_1, 0xffffffff);
    PUT32(INTERRUPT_DISABLE_2, 0xffffffff);
    system_disable_interrupts();
    cp15_barrier();

    // setup the interrupt vectors.
    install_handlers();
    int_intialized_p = 1;
    swi_setup_stack(SWI_STACK_ADDR);

    cpsr_print_mode(cpsr_read_c());
    // assert(cpsr_read_c() == SYS_MODE);
}

// you will call this with the pc of the SWI instruction, and the saved registers
// in saved_regs.  r0 at offset 0, r1 at offset 1, etc.
void handle_swi(uint8_t sysno, uint32_t pc, uint32_t *saved_regs) {
    printk("sysno=%d\n", sysno);
    printk("\tcpsr =%x\n", cpsr_read());
    assert(cpsr_read_c() == SUPER_MODE);

    // check that the stack is in-bounds.
    int i;
    assert(&i < (int*)SWI_STACK_ADDR);

    printk("\treturn=%x stack=%x\n", pc, saved_regs);
    printk("arg[0]=%d, arg[1]=%d, arg[2]=%d, arg[3]=%d\n",
           saved_regs[0],
           saved_regs[1],
           saved_regs[2],
           saved_regs[3]);

    switch (sysno) {
        case 1:
            saved_regs[0] = (uint32_t) kernel_sbrk(saved_regs[0]);
            break;
    }
}
