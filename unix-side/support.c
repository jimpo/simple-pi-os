#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "demand.h"
#include "simple-boot.h"
#include "support.h"

// read entire file into buffer.  return it, write total bytes to <size>
unsigned char *read_file(int *size, const char *name) {
    int ret;
    int fd = open(name, O_RDONLY);
    if (fd < 0) {
        panic("Failed to open file");
    }

    struct stat statbuf;
    ret = fstat(fd, &statbuf);
    if (ret < 0) {
        panic("Failed to stat file");
    }

    *size = (statbuf.st_size + 3) / 4 * 4;
    void* buffer = malloc(*size);
    if (!buffer) {
        panic("Failed to allocate buffer");
    }
    memset(buffer, 0, *size);

    int n_read = read(fd, buffer, statbuf.st_size);
    if (n_read < 0) {
        panic("Failed to read file");
    }
    if (n_read != statbuf.st_size) {
        panic("Failed to read complete file");
    }

    ret = close(fd);
    if (ret < 0) {
        panic("Failed to close file");
    }
    return (unsigned char*)buffer;
}

#define _SVID_SOURCE
#include <dirent.h>
const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
	"tty.SLAB_USB", // mac os
	0
};

int filter_for_usb(const struct dirent* entry) {
    const char** p_prefix;
    for (p_prefix = ttyusb_prefixes; *p_prefix; p_prefix++) {
        size_t len = strnlen(*p_prefix, 256);
        if (strncmp(*p_prefix, entry->d_name, len) == 0) {
            return true;
        }
    }
    return false;
}

// open the TTY-usb device:
//	- use <scandir> to find a device with a prefix given by ttyusb_prefixes
//	- returns an open fd to it
// 	- write the absolute path into <pathname> if it wasn't already
//	  given.
int open_tty(const char **portname) {
    char dev_dir[] = "/dev";

    struct dirent** namelist = NULL;
    int entries = scandir(dev_dir, &namelist, filter_for_usb, alphasort);
    if (entries < 0) {
        panic("Failed to scan /dev directory");
    }
    if (entries == 0) {
        panic("No USB devices found");
    }

    struct dirent* first_entry = *namelist;
    size_t dir_len = strnlen(dev_dir, 256);
    size_t file_len = strnlen(first_entry->d_name, 256);
    size_t len = dir_len + 1 + file_len + 1;
    char* path = (char*) malloc(len * sizeof(char));
    int ret = snprintf(path, len, "%s/%s", dev_dir, first_entry->d_name);
    if (ret < 0) {
        panic("Failed to construct file path");
    }

    int i;
    for (i = 0; i < entries; i++) {
        free(namelist[i]);
    }
    free(namelist);

    int fd = open(path, O_RDWR|O_NOCTTY|O_SYNC);
    if (fd < 0) {
        panic("Failed to open file");
    }

    *portname = path;
	return fd;
}
