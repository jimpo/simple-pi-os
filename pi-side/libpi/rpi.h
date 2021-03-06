/*
 * General functions we use.  These could be broken into multiple small
 * header files, but that's kind of annoying to context-switch through,
 * so we put all the main ones here.
 */
#ifndef __RPI_H__
#define __RPI_H__

/*****************************************************************************
 * math
 */

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define true 1
#define false 0

// Determine base 2 log of an integer greater than 0 rounded down to the nearest integer.
unsigned int_log2(unsigned v);

/*****************************************************************************
 * pi types
 */

#ifndef FAKE_PI
#   include <stddef.h>
#   include <stdint.h>
#if 0
    // if we are compiling fake, then I think we want to override this.
    typedef unsigned long long uint64_t;
    typedef unsigned uint32_t;
    typedef unsigned short uint16_t;
    typedef unsigned char uint8_t;
    typedef short int16_t;
    typedef int int32_t;
    typedef signed char int8_t;
    typedef unsigned size_t;
    typedef unsigned* uintptr_t;
#	define offsetof(st, m) __builtin_offsetof(st, m)
#endif

// shouldn't link these in if running on linux?  these conflict
int strcmp(const char *_p, const char *q);
void *memset(void *_p, int c, size_t n) ;
int memcmp(const void *_s1, const void *_s2, size_t nbytes);
void *memcpy(void *dst, const void *src, size_t nbytes);
int strncmp(const char* _s1, const char* _s2, size_t n);
char *strcat (char *dest, const char *src);
size_t strlen(const char *p);
char *strcpy(char * s1, const char * s2);
char* strchr(const char* s, int c);
int toupper(int c);
#else
#	include <stddef.h>
#endif

/*****************************************************************************
 * common device functions
 */
// uart functions
void uart_init(void);
int uart_getc(void);
void uart_putc(unsigned c);

// simple timer functions.
void delay_cycles(unsigned ticks) ;
unsigned timer_get_time(void) ;
void delay_us(unsigned us) ;
void delay_ms(unsigned ms) ;

// a not very good rand()
unsigned short rpi_rand(void);

/*****************************************************************************
 * standard libc like functions for the pi.
 */
int printk(const char *format, ...);
int snprintk(char *buf, size_t n, const char *fmt, ...);
int putk(const char *msg);

int rpi_putchar(int c);

// reboot the pi.
void rpi_reboot(void) __attribute__((noreturn));

// reboot after printing out a string to cause the unix my-install to shut down.
void clean_reboot(void) __attribute__((noreturn));

// set pc value to <addr>
void BRANCHTO( unsigned addr);

// a no-op routine called to defeat the compiler.
void dummy(unsigned);


/*******************************************************************************
 * simple memory allocation.
 */

void kfree(void *p);
void kfree_all(void);
// returns 0-filled memory aligned to <nbits_alignment>
void *kmalloc_aligned(unsigned nbytes, unsigned nbits_alignment);
// returns 0-filled memory.
void *kmalloc(unsigned nbytes) ;

// set where the heap starts and ends.
void kmalloc_init(unsigned start, unsigned end);

/*****************************************************************************
 * memory barriers
 */
void mb(void);
void dmb(void);
void dsb(void);

// use this if you need memory barriers.
void dev_barrier(void);


/*****************************************************************************
 * put/get from memory in a way that we can override easily + defeat compiler.
 */
// *(unsigned *)addr = v;
void PUT32(unsigned addr, unsigned v);
void put32(volatile void *addr, unsigned v);

// *(unsigned char *)addr = v;
void PUT16(unsigned addr, unsigned short v);
void put16(volatile void *addr, unsigned short v);

// *(unsigned char *)addr = v;
void PUT8(unsigned addr, unsigned char v);
void put8(volatile void *addr, unsigned char v);

// *(unsigned *)addr
unsigned GET32(unsigned addr);
unsigned get32(const volatile void *addr);

// *(unsigned short *)addr
unsigned short GET16(unsigned addr);
unsigned short get16(const volatile void *addr);

// *(unsigned char *)addr
unsigned char GET8(unsigned addr);
unsigned char get8(const volatile void *addr);

// jump to <addr>
void BRANCHTO(unsigned addr);

// cache enable
void enable_cache(void) ;
void disable_cache(void) ;

unsigned rpi_get_cpsr(void);

uint32_t crc32(const void *buf, unsigned size);

#include "gpio.h"
#include "assert.h"

#endif
