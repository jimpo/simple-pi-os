#include "libpi/rpi.h"

int _start() {
    char message[] = "hello world!";

    void* addr = kmalloc(1024);
    memcpy(addr, message, sizeof(message));
    printk("%s\n", addr);
    kfree_all();

    return 0;
}