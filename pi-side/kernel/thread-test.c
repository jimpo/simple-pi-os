/*
 * trivial code to test a simple threads package.
 * implement:
 *	- part0()
 *	- then part1()
 *	- then part2()
 * then do additional tests.
 */
#include "libpi/rpi.h"
#include "rpi-thread.h"

#define USER_MODE 0x10
#define IRQ_MODE 0x11
#define FIQ_MODE 0x12
#define SUPERVISOR_MODE  0x13
#define ABORT_MODE  0x17
#define UNDEFINED_MODE  0x1B
#define SYSTEM_MODE  0x1F

// implement these: just pass in constants for <a1>, <a2>, <a3> to verify
// these registers got saved.  check the values printed for the pc, lr and
// verify they make sense (how?)
unsigned *test_csave(uint32_t *u, unsigned a1, unsigned a2, unsigned a3);
unsigned *test_csave_stmfd(uint32_t *u, unsigned a1, unsigned a2, unsigned a3);

void part0(void) {
	printk("running part0\n");
	unsigned cpsr = rpi_get_cpsr();
	printk("cpsr() = %x\n", cpsr);

	// check that interrupts are disabled and that we are in kernel mode.
    unsigned irq_int_disabled = (cpsr >> 7) & 1;
    unsigned mode = (cpsr >> 0) & 0x1F;
    assert(irq_int_disabled);
    assert(mode == SUPERVISOR_MODE);

	// stack grows down.
	unsigned u[128+1], *e = &u[127], n, *p;
	u[127] = 0;
	u[128] = 0;
	printk("end of save=%p\n", &u[127]);

    p = test_csave(e, 1,2,3) + 1;
    n = e - p + 1;
    printk("save area = %x, saved [%d] registers, %d bytes\n", e, n, 4*n);
	for(int i = 0; i < n; i++)
		printk("\tr[%d] = %x\n", i, p[i]);

	assert(!u[128]);

	p = test_csave_stmfd(e, 1,2,3) + 1;
	n = e - p + 1;
	printk("save area = %x, saved [%d] registers, %d bytes\n", e, n, 4*n);
	for(int i = 0; i < n; i++)
		printk("\tr[%d] = %x\n", i, p[i]);
	assert(!u[128]);
}

// called from assembly.
void notreached(void) { 
	panic("not reachined was reached!\n"); 
}

void part1(void) {
	rpi_thread_t t;
	memset(&t, 0, sizeof t);

	printk("running part1\n");
	// check that we can context switch to ourselves multiple
	// times.
	for(int i = 0; i < 100; i++) {
		printk("%d: about to cswitch, addr of sp=%x\n", i, &t.sp);
		unsigned r = rpi_cswitch(&t.sp, &t.sp);
		printk("%d: returned=%x, sp=%x\n", i, r, t.sp);
		printk("\tgetcpsr=%x\n", rpi_get_cpsr());
	}
}

// trivial test.   you should write a few that are better, please!
volatile int thread_count, thread_sum;
static void thread_code(void *arg) {
	printk("in thread %p, with %x\n", rpi_cur_thread()->tid, arg);
	thread_count ++;
	thread_sum += (unsigned)arg;
    rpi_yield();
	rpi_yield();
	rpi_yield();
	rpi_yield();
	rpi_yield();
	rpi_yield();
	rpi_yield();
	rpi_yield();
}

// check that we can fork/yield/exit and start the threads package.
void part2(void) {
	printk("running part2\n");
	int n = 30;
	thread_sum = thread_count = 0;
	for(int i = 0; i < n; i++) {
		rpi_fork(thread_code, (void*)i);
    }
	rpi_thread_start(0);

	// no more threads: check.
	printk("count = %d, sum=%d\n", thread_count, thread_sum);
	assert(thread_count == n);
}

void notmain() {
    uart_init();

    /* part0(); */
    /* part1(); */
    for (int i = 0; i < 3; i++) {
        part2();
    }
    clean_reboot();
}
