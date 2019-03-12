#include <libpi/rpi.h>

#include "rpi-thread.h"

// typedef rpi_thread_t E;
#define E rpi_thread_t
#include "Q.h"

#define WORD_SIZE 4

static struct Q runq, freeq;
static unsigned nthreads = 0;

static rpi_thread_t *cur_thread;  // current running thread.
static rpi_thread_t *scheduler_thread; // first scheduler thread.

// call kmalloc if no more blocks, otherwise keep a cache of freed threads.
static rpi_thread_t *mk_thread(void) {
	rpi_thread_t *t;

	if(!(t = Q_pop(&freeq)))
 		t = kmalloc(sizeof(rpi_thread_t));

	return t;
}

// create a new thread.
rpi_thread_t *rpi_fork(void (*code)(void *arg), void *arg) {
	rpi_thread_t *t = mk_thread();

	// you can use these for setting the values in the first thread.
	// if your context switching code stores these values at different
	// stack offsets, change them!
	enum {
		// register offsets are in terms of byte offsets!
		R0_offset = 0x04 / WORD_SIZE,
		R1_offset = 0x08 / WORD_SIZE,
		LR_offset = 0x38 / WORD_SIZE,
		CPSR_offset = 0x3C / WORD_SIZE,
        TOTAL_offset = 0x3C / WORD_SIZE,
	};

	// write this so that it calls code,arg.
	void rpi_init_trampoline(void);

	// do the brain-surgery on the new thread stack here.
    size_t stack_size = sizeof(t->stack) / sizeof(*t->stack);
    t->tid = nthreads++;
    t->sp = t->stack + stack_size - 1;
    t->sp -= TOTAL_offset;
    t->sp[CPSR_offset] = rpi_get_cpsr();
    t->sp[R0_offset] = (uint32_t) arg;
    t->sp[R1_offset] = (uint32_t) code;
    t->sp[LR_offset] = (uint32_t) rpi_init_trampoline;

	Q_append(&runq, t);
	return t;
}

// exit current thread.
void rpi_exit(int exitcode) {
    // 1. free current thread.
    memset(cur_thread, 0, sizeof(*cur_thread));
    Q_append(&freeq, cur_thread);
    cur_thread = NULL;

    // 2. if there are more threads, dequeue one and context
    // switch to it.
    cur_thread = Q_pop(&runq);

    // 3. otherwise we are done, switch to the scheduler thread
    // so we call back into the client code.
    if (!cur_thread) {
        cur_thread = scheduler_thread;
    }

    uint32_t* dummy;
    rpi_cswitch(&dummy, &cur_thread->sp);
}

// yield the current thread.
void rpi_yield(void) {
    // if cannot dequeue a thread from runq
    //	- there are no more runnable threads, return.
    rpi_thread_t* next_thread = Q_pop(&runq);
    if (!next_thread) {
        return;
    }

	// otherwise:
	//	1. enqueue current thread to runq.
    rpi_thread_t* last_thread = cur_thread;
    Q_append(&runq, last_thread);

	// 	2. context switch to the new thread.
    cur_thread = next_thread;
    rpi_cswitch(&last_thread->sp, &cur_thread->sp);
}

// starts the thread system: nothing runs before.
// 	- <preemptive_p> = 1 implies pre-emptive multi-tasking.
void rpi_thread_start(int preemptive_p) {
	assert(!preemptive_p);

	// if runq is empty, return.
	// otherwise:
	//    1. create a new fake thread
	//    2. dequeue a thread from the runq
	//    3. context switch to it, saving current state in
 	//	  <scheduler_thread>

    cur_thread = Q_pop(&runq);
    if (!cur_thread) {
        return;
    }

	scheduler_thread = mk_thread();
    rpi_cswitch(&scheduler_thread->sp, &cur_thread->sp);

	printk("THREAD: done with all threads, returning\n");
}

// pointer to the current thread.
//	- note: if pre-emptive is enabled this can change underneath you!
rpi_thread_t *rpi_cur_thread(void) {
	return cur_thread;
}

// call this an other routines from assembler to print out different
// registers!
void check_regs(unsigned r0, unsigned r1, unsigned sp, unsigned lr) {
	printk("r0=%x, r1=%x lr=%x cpsr=%x\n", r0, r1, lr, sp);
	clean_reboot();
}
