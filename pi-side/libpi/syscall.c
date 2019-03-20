#include "syscall.h"

static void* (*sbrk)(unsigned);

void rpi_set_sbrk(void* (*_sbrk)(unsigned)) {
    sbrk = _sbrk;
}

void* rpi_sbrk(unsigned incr) {
    return sbrk(incr);
}
