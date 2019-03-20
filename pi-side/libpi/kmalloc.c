#include "rpi.h"
#include "syscall.h"


#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))
#define is_aligned(x, a)        (((x) & ((typeof(x))(a) - 1)) == 0)
union align {
        double d;
        void *p;
        void (*fp)(void);
};


static char* heap_start = 0;
static char* heap_end = 0;
static char* heap = 0;

void *kmalloc(unsigned sz) {
    sz = roundup(sz, sizeof(union align));

    if (!heap_start) {
        heap = heap_start = heap_end = rpi_sbrk(0);
    }
    while (heap + sz > heap_end) {
        unsigned incr = roundup(heap + sz - heap_end, 1 << 20);
        assert(rpi_sbrk(incr) == heap_end + incr);
        heap_end += incr;
    }
    void *addr = heap;
    heap += sz;

    memset(addr, 0, sz);
    return addr;
}

#define is_pow2(x)  (((x)&-(x)) == (x))

void *kmalloc_aligned(unsigned nbytes, unsigned alignment) {
    demand(is_pow2(alignment), assuming power of two);

    // XXX: we waste the alignment memory.  aiya.
    //  really should migrate to the k&r allocator.
    unsigned h = (unsigned)heap;
    h = roundup(h, alignment);
    demand(is_aligned(h, alignment), impossible);
    heap = (void*)h;

    return kmalloc(nbytes);
}

void kfree(void *p) { }
void kfree_all(void) { heap = heap_start; };
void kfree_after(void *p) { heap = p; }

