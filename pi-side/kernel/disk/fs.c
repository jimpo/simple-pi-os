/* simple fat32 read-only fs. */
#include <libpi/helper-macros.h>
#include <libpi/rpi.h>

#include "fat32.h"
#include "fs.h"
#include "sd.h"

#define SECTOR_SIZE 512

// allocate <nsec> worth of space, read in from SD card, return pointer.
void *sec_read(uint32_t lba, uint32_t nsec) {
    void *data = kmalloc(SECTOR_SIZE * nsec);
    int result = sd_readblock(lba, data, nsec);
    demand(result == nsec * SECTOR_SIZE, failed to read block);
    return data;
}

// there are other tags right?
static int is_fat32(int t) { return t == 0xb || t == 0xc; }

typedef struct fat32 {
    uint32_t fat_begin_lba,
            cluster_begin_lba,
            sectors_per_cluster,
            root_dir_first_cluster;
    uint32_t *fat;
    uint32_t n_fat;
} fat32_fs_t;

/*
    Field                       Microsoft's Name    Offset   Size        Value
    Bytes Per Sector            BPB_BytsPerSec      0x0B(11) 16 Bits     Always 512 Bytes
    Sectors Per Cluster         BPB_SecPerClus      0x0D(13) 8 Bits      1,2,4,8,16,32,64,128
    Number of Reserved Sectors  BPB_RsvdSecCnt      0x0E(14) 16 Bits     Usually 0x20
    Number of FATs              BPB_NumFATs         0x10(16) 8 Bits      Always 2
    Sectors Per FAT             BPB_FATSz32         0x24(36) 32 Bits     Depends on disk size
    Root Directory First ClusterBPB_RootClus        0x2C(44) 32 Bits     Usually 0x00000002
    Signature                   (none)              0x1FE(510)16 Bits     Always 0xAA55
*/
static struct fat32 fat32_mk(uint32_t lba_start, fat32_boot_sec_t *b) {
    struct fat32 fs;

    fs.fat_begin_lba = lba_start + b->reserved_area_nsec;
    fs.cluster_begin_lba = fs.fat_begin_lba + b->nfats * b->nsec_per_fat;
    fs.sectors_per_cluster = b->sec_per_cluster;
    fs.root_dir_first_cluster = b->first_cluster;
    fs.n_fat = b->nfats;

    // read in the both copies of the FAT.

    uint32_t* fat1 = (uint32_t*)sec_read(fs.fat_begin_lba, b->nsec_per_fat);
    uint32_t* fat2 = (uint32_t*)sec_read(fs.fat_begin_lba + b->nsec_per_fat, b->nsec_per_fat);

    // check that they are the same.
    assert(memcmp(fat1, fat2, b->nsec_per_fat) == 0);

    fs.fat = fat1;
    kfree(fat2);
    return fs;
}

// given cluster_number get lba
uint32_t cluster_to_lba(struct fat32 *f, uint32_t cluster_num) {
    return f->cluster_begin_lba + (cluster_num - f->root_dir_first_cluster) * f->sectors_per_cluster;
}

int fat32_dir_cnt_entries(fat32_dir_t *d, int n) {
    int cnt = 0;
    for(int i = 0; i < n; i++, d++)
        if(!fat32_dirent_free(d) && d->attr != FAT32_LONG_FILE_NAME)
                cnt++;
    return cnt;
}

// translate fat32 directories to dirents.
pi_dir_t fat32_to_dirent(fat32_fs_t *fs, fat32_dir_t *d, uint32_t n) {
    pi_dir_t p;
    p.n = fat32_dir_cnt_entries(d,n);
    p.dirs = kmalloc(p.n * sizeof *p.dirs);

    int used = 0;
    fat32_dir_t* end = d + n;
    for(; d < end; d++) {
        if(!fat32_dirent_free(d)) {
            dirent_t *de = &p.dirs[used++];
            assert(used <= p.n);

            d = fat32_dir_filename(de->name, d, end);
            de->cluster_id = (d->hi_start << 16) + d->lo_start;
            de->is_dir_p = !!(d->attr & FAT32_DIR);
            de->nbytes = d->file_nbytes;
            de->d = 0;
            de->f = 0;
        }
    }

    return p;
}

// read in an entire file.  note: make sure you test both on short files (one cluster)
// and larger ones (more than one cluster).
pi_file_t fat32_read_file(fat32_fs_t *fs, dirent_t *d) {
    // the first cluster.
    uint32_t start_id = d->cluster_id;
    pi_file_t f;

    // compute how many clusters in the file.
    //      note: you know you are at the end of a file when
    //          fat32_fat_entry_type(fat[id]) == LAST_CLUSTER.
    // compute how many sectors in cluster.
    // allocate this many bytes.

    uint32_t n_clusters = 0;
    for (uint32_t id = start_id;
         fat32_fat_entry_type(id) != LAST_CLUSTER;
         id = fs->fat[id]) {
        n_clusters++;
    }

    uint32_t n_sectors = n_clusters * fs->sectors_per_cluster;

    f.n_data = d->nbytes;
    f.n_alloc = n_sectors * SECTOR_SIZE;
    f.data = (char*)kmalloc(f.n_alloc);

    char* data = f.data;
    for (uint32_t id = start_id;
         fat32_fat_entry_type(id) != LAST_CLUSTER;
         id = fs->fat[id]) {
        int bytes_read = sd_readblock(cluster_to_lba(fs, id), (unsigned char*)data, fs->sectors_per_cluster);
        demand(bytes_read == fs->sectors_per_cluster * SECTOR_SIZE, failed to read block);
        data += bytes_read;
    }

    return f;
}

dirent_t *dir_lookup(pi_dir_t *d, char *name) {
    for(int i = 0; i < d->n; i++) {
        dirent_t *e = &d->dirs[i];
        if(strcmp(name, e->name) == 0)
            return e;
    }
    return 0;
}
