/* 
 * part1: trivial file system that we are going to repurpose for the pi.  
 * 
 * useful helper routines in pi-fs.h/pi-fs-support.c
 *
 * implement anything with unimplemented();
 */

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>
#include <support/demand.h>

#include "fs.h"
#include "redirect.h"
#include "echo-until.h"

#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

static int do_reboot(dirent_t *e, const char *path, const char *buf,
                            size_t size, off_t offset, void *data);
static int do_echo(dirent_t *e, const char *path, const char *buf,
                            size_t size, off_t offset, void *data);
static int do_run(dirent_t *e, const char *path, const char *buf,
                            size_t size, off_t offset, void *data);

// simple pi file system: no directories.
static dirent_t root[] = {
    { .name = "/echo.cmd", perm_rw, 0, .on_wr = do_echo },
    { .name = "/reboot.cmd", perm_rw, 0, .on_wr = do_reboot},
    { .name = "/run.cmd", perm_rw, 0, .on_wr = do_run},
    { .name = "/console", perm_rd, 0 },
    { 0 }
};

static int pi_open(const char *path, struct fuse_file_info *fi) {
    note("opening\n");

    int retv;
    dirent_t *e = file_lookup(&retv, root, path, flag_to_perm(fi->flags));
    if(!e) {
        return retv;
    }

    return 0;
}

static int pi_readdir(const char *path, void *buf, fuse_fill_dir_t filler, 
                            off_t offset, struct fuse_file_info *fi) {
    note("readdir\n");
    if(strcmp(path, "/") != 0)
        return -ENOENT;

    // no sub-directories
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (dirent_t* e = root; e->name; e++) {
        char* name = e->name + 1; // Skip the leading /
        filler(buf, name, NULL, 0);
    }
    return 0;
}

static int pi_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path,"/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        // we need one link for each subdir since they point to us.
        // plus 2 for "." and parent (or ".." in the case of "/").
        stbuf->st_nlink = 2;
        return 0;
    }

    // regular file.
    dirent_t *e = ent_lookup(root, path);
    if(!e) {
        return -ENOENT;
    }
    stbuf->st_mode = S_IFREG | e->flags;
    stbuf->st_nlink = 1;
    stbuf->st_size = e->f->n_data;

    return 0;
}

static int pi_read(const char *path, char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {

    note("****** READ\n");
    int retv;
    dirent_t *e = file_lookup(&retv, root, path, perm_rd);
    if(!e) {
        return retv;
    }

    if (offset > e->f->n_data) {
        return 0;
    }

    size = MIN(size, e->f->n_data - offset);
    memcpy(buf, e->f->data + offset, size);
    return size;
}

int append_to_file(file_t* f, const char* buf, size_t size, off_t offset) {
    if (offset + size > f->n_alloc) {
        char* new_data = (char*) realloc(f->data, offset + size);
        if (!new_data) {
            return -errno;
        }
        f->data = new_data;
        f->n_alloc = offset + size;
    }

    memcpy(f->data + offset, buf, size);
    f->n_data = MAX(f->n_data, offset + size);
    return size;
}

static int pi_write(const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fi) {

    note("****** WRITE:<%s> off=%ld, size=%ld\n", path,offset,size);
    int retv;
    dirent_t *e = file_lookup(&retv, root, path, perm_wr);
    if(!e) {
        return retv;
    }

    if (e->on_wr) {
        return e->on_wr(e, path, buf, size, offset, NULL);
    }

    if (offset > e->f->n_data) {
        return 0;
    }

    return append_to_file(e->f, buf, size, offset);
}

/** Change the size of a file */
static int pi_truncate(const char *path, off_t length) {
    int retv;
    dirent_t *e = file_lookup(&retv, root, path, perm_wr);
    if(!e) {
        return retv;
    }

    file_t* f = e->f;
    if (length < f->n_data) {
        f->n_data = length;
    }
    return 0;
}

static int
pi_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    return pi_truncate(path, offset);
}

/* these were enough for me for the lab. */
static struct fuse_operations pi_oper = {
    .getattr = pi_getattr, 
    .readdir = pi_readdir,
    .open = pi_open, 
    .read = pi_read,
    .write = pi_write,
    .truncate = pi_truncate,
    .ftruncate = pi_ftruncate,
};

enum { TRACE_FD_REPLAY = 11, TRACE_FD_HANDOFF };
static int is_open(int fd) {
    return fcntl(fd, F_GETFL) >= 0;
}
static dirent_t *console = 0;
static int using_pi_p = 0;
static int pi_rd_fd, pi_wr_fd;
static char prompt[] = "PIX:> ";

int wr_console(int i) {
    char c = i;
    append_to_file(console->f, &c, 1, console->f->n_data);
    fputc(c, stderr);
    return 0;
}

/*
    -d Enable debugging output (implies -f).

    -f Run in foreground; this is useful if you're running under a
    debugger. WARNING: When -f is given, Fuse's working directory is
    the directory you were in when you started it. Without -f, Fuse
    changes directories to "/". This will screw you up if you use
    relative pathnames.

    -s Run single-threaded instead of multi-threaded.
*/
int main(int argc, char *argv[]) {
    assert(ent_lookup(root, "/console"));
    assert(ent_lookup(root, "/echo.cmd"));
    assert(ent_lookup(root, "/reboot.cmd"));
    assert(ent_lookup(root, "/run.cmd"));

    console = ent_lookup(root, "/console");

    char *pi_prog = 0;
    if(argc > 3 && strcmp(argv[argc-2], "-pi") == 0) {
        assert(!argv[argc]);
        pi_prog = argv[argc-1];
        argc -= 2;
        argv[argc] = 0;
        using_pi_p = 1;
        demand(is_open(TRACE_FD_HANDOFF), no handoff?);
        note("pi prog=%s\n", pi_prog);
        if(redir(&pi_rd_fd, &pi_wr_fd, pi_prog) < 0)
            panic("redir failed\n");
        // have to give the pi time to get up.
        sleep(1);
        echo_until_fn(pi_rd_fd, prompt, wr_console);
    } else
        demand(!is_open(TRACE_FD_HANDOFF), no handoff?);


    return fuse_main(argc, argv, &pi_oper, 0);
}

// not needed for this part.
static int do_reboot(dirent_t *e, const char *path, const char *buf,
                            size_t size, off_t offset, void *data) {
    if (offset != 0) {
        // This is bad
    }

    fd_puts(pi_wr_fd, "reboot\n");
    echo_until_fn(pi_rd_fd, prompt, wr_console);
    exit(0);
}

static int do_echo(dirent_t *e, const char *path, const char *buf,
                            size_t size, off_t offset, void *data) {
    if (offset != 0) {
        // This is bad
    }

    fd_puts(pi_wr_fd, "echo ");
    fd_puts(pi_wr_fd, buf);
    fd_puts(pi_wr_fd, "\n");
    echo_until_fn(pi_rd_fd, prompt, wr_console);
    return size;
}

static int do_run(dirent_t *e, const char *path, const char *buf,
                            size_t size, off_t offset, void *data) {
    fd_puts(pi_wr_fd, buf);
    fd_puts(pi_wr_fd, "\n");
    echo_until_fn(pi_rd_fd, prompt, wr_console);
    return size;
}
