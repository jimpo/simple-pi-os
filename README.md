# Simple Pi OS

This is a partial operating system for a bare metal Rasberry Pi A+, written for [Stanford CS 140e](http://web.stanford.edu/class/cs140e/).

## Features

### Bootloader

The operating system kernel is loaded into the Pi's memory from the bootloader over the UART interface on each reboot. The kernel implements virtual memory, listing directories and reading files on the SD card, and execution of programs read from disk.

### Memory layout

The first 1 MiB of physical memory starting at 0x00000000 is reserved for the kernel and will be mapped into every process's virtual address space at the same address. Any addresses corresponding to BCM2835 peripherals, like GPIO pins, are mapped to the same virtual addresses as their physical addresses (starting with 0x20) in every process.

The kernel's virtual address space layout is:

- 0x00000000-???: The table of exeception vectors and implementation of basic interrupt handlers.
- ???-0x00008000: The kernel stack, which is shared with child processes.
- 0x00008000-???: The kernel text, data, and bss segments.
- 0x000D0000-0x000E0000: The stack during general interrupt handling.
- 0x000E0000-0x000F0000: The stack during software interrupt handling.
- 0x00100000-0x00050000: Space allocated for 256 page tables, 16 KiB each, one per available ASID.
- 0x00500000-???: Kernel heap space.
- 0x10000000-???: Kernel program loading buffer.

Each process's virtual address space is determined dynamically by the ELF headers, with the exception that the first 1 MiB is reserved for the kernel code as well as some sections in the range of peripheral addresses.

### Virtual memory

The virtual memory system uses 1 MiB sections and supports multiple processes with different ASIDs. No access control is implemented and all processes use the same domain. The kernel keeps track of which physical pages are in use in a global map to avoid collisions. This is necessary since pages are not mapped to disk locations, just physical memory, and there is no swapping.

### User Processes

Processes are identified by an 8-bit process ID, which are the same as their ASIDs. Each process has its own virtual address space. The kernel's structure for a process tracks its page translations. A page table structure references the actual array of first level descriptors read by the MMU.

```c
typedef struct {
    unsigned heap_start;
    unsigned heap_end;
    page_table_t page_tab;
} process_t;
```

The kernel runs with ASID 1 because ASID 0 is reserved as unused. This is a trick so that the instruction TLB is not filled with incorrectly prefetched sections while a TTBR change is occurring. User processes currently have no strong protection and run in the same mode as the kernel.

### Program execution

This is the main differentiating feature from the code in labs. The kernel is able to execute statically linked ELF32 files. The steps for executing a new process are:

- The kernel initializes a new process structure and assigns it an ASID.
- The kernel zeros out the corresponding page table, then maps in the global pages. Namely, 0x00000000 points to the kernel section and 0x20?????? maps to peripherals.
- The kernel maps the a new physical section to 0x10000000, the program load region, and reads the entire executable file contents there. This new page will be shared by the child process.
- The kernel then parses the ELF header, program headers, and section headers. It searches for any program header sections that need to be copied into the child processes address space and sets up a new mapping in the child's page table to a free section of physical memory. It does not, however, copy the segments yet since there is no mapping to the page in the kernel. Instead the load logic constructs a list of memory operations to be executed after the ASID and TTBR are switched. The kernel also searches for the bss segments, creates page table mappings, and adds memory operations to zero them out. The loader then identifies the starting address of the heap as the lowest address after all data and bss segments. Finally, the ELF logic identifies the text segment and returns the starting address.
- Now the kernel switches the ASID and TTBR to the child process.
- It then executes the memory operations (copying data and text segments from the load area to the appropriate addresses, zeroing bss segments, etc.). Finally it branches to the beginning of the process's text segment.
- Notably, the stack pointer is not changed -- the child process's shares a stack with the kernel. This is simpler and since the kernel stack is mapping into the child process anyway, moving it doesn't feel that important.
- When the executable returns control back to the kernel, it switches the TTBR back to the kernel's page table and the ASID back as well. It then issues TLB invalidate commands through the CP15 register to invalidate all TLB entries with the ASID of the exited child process. This allows the ASID to be reused by another process in the future.
- The last step is to go through the exited child process's page table and mark all physical sections that it owned as free in the global mapping.

### Syscalls

The kernel currently implements one syscall, `sbrk`, which works similarly to the Unix `sbrk`. A process can call it to request additional heap space from the kernel. The kernel keeps global pointers to the currently executing process, which stores the start and end addresses of its heap. `sbrk` can then switch back to the kernel's page table, ensure that the new virtual address space dedicated to the heap is mapped to free physical sections, and switch the page table back. This is a bit tricky because we need to clean the cache line of the modified page table entry and invalidate the TLB entries so that the changes are visible to the MMU. One thing that took a lot of time to deal with is that the CPSR mode that the Pi starts up in seems to be non-deterministic, and I haven't figured out why yet.

## Running the OS

### Dependencies
- [CMake](https://cmake.org/)
- [FUSE](https://github.com/libfuse/libfuse)
- [GNU Arm Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)

On Ubuntu/Debian:
```bash
$ sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
$ sudo apt-get update
$ apt-get install gcc fuse libfuse-dev make cmake
```

### Installation

- Copy the files in `firmware/` to the root directory of the Pi's SD card ([source](https://github.com/raspberrypi/firmware)).
- Build the project
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```
- Build and copy `pi-side/bootloader/bootloader.bin` to the root directory of the Pi's SD card.
- Run `make run-shell` to interact with the device.

## Using the shell

The shell supports the following builtin commands:

- `ls PATH`. List a directory by absolute path beginning with a forward slash.
- `cat PATH`. Print a file on disk by absolute path beginning with a forward slash.
- `run PATH`. Run a statically-linked ELF executable file by absolute path.
- `reboot`. Reboot the Pi.

## Writing programs

Userspace programs to be run on the Pi are located in `pi-side/usr/`. The `CMakeFiles.txt` examples are very simple and produce statically linked executables. One thing to be aware of is that the first 1 MiB of the address space in these programs is reserved for the kernel, so the text segment has to start at 0x10000000 at least.

## References

[BCM ARM Peripherals](https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)
