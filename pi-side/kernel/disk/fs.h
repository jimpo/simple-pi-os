#ifndef __DISK_FS_H__
#define __DISK_FS_H__

// from the fuse lab.
//
// this way of creating a ramFS is not the right way to do things.
// we use expedient hacks so we can strip the things down for a 
// couple hour lab.
typedef struct {
    char *data;
    unsigned cksum;
    size_t n_alloc,    // total bytes allocated.
             n_data;     // how many bytes of data
} pi_file_t;

typedef struct {
    struct dirent *dirs;
    uint32_t n;
} pi_dir_t;

// for first lab: no directories.
typedef struct dirent {
#   define MAXNAME 256
    char name[MAXNAME];
    uint32_t cluster_id;       // location on disk.
    unsigned is_dir_p:1;
    uint32_t nbytes;

    // if we've read it in.
    pi_dir_t *d;
    pi_file_t *f;

    // int flags;      // not sure how to translate fat32 to permissions.
} dirent_t;

typedef struct fat32 {
    uint32_t fat_begin_lba,
            cluster_begin_lba,
            sectors_per_cluster,
            root_dir_first_cluster;
    uint32_t *fat;
    uint32_t n_fat;
} fat32_fs_t;

fat32_fs_t fat32_init();
pi_dir_t fat32_read_dir(fat32_fs_t *fs, dirent_t *d);
pi_file_t fat32_read_file(fat32_fs_t *fs, dirent_t *d);

// lookup <path> in <search_dir>
// if return value is NULL, path was not found
// if return value is not NULL, last_dir is valid and must be freed by caller
dirent_t *path_lookup(fat32_fs_t* fs, dirent_t *search_dir, const char *path, pi_dir_t* last_dir);

#if 0
// allocate a new file structure, initial file size =<hint>
file_t *file_new(size_t hint) ;
// reallocate the file with file stucture to at least <needed>
void file_realloc(pi_file_t *f, size_t needed);

// lookup a path in <d>: any error code is written to <retv>
dirent_t * file_lookup(int *retv, dirent_t *d, const char *path, int perm);
#endif
#endif
