#include <string.h>             // memcpy
#include "diskimg.h"            // diskimg_readsector, DISKIMG_SECTOR_SIZE
#include "inode.h"              // struct inode
#include "unixfilesystem.h"     // struct unixfilesystem, INODE_START_SECTOR
#include "direntv6.h"           // para el typedef de addr_t implícito

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    int inodes_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct inode);
    int max_inodes       = fs->superblock.s_isize * inodes_per_block;
    if (inumber < 1 || inumber > max_inodes) {
        return -1;
    }

    int idx    = inumber - 1;
    int sector = INODE_START_SECTOR + (idx / inodes_per_block);
    int offset = (idx % inodes_per_block) * sizeof(struct inode);

    char buf[DISKIMG_SECTOR_SIZE];
    int n = diskimg_readsector(fs->dfd, sector, buf);
    if (n != DISKIMG_SECTOR_SIZE) {
        return -1;
    }

    memcpy(inp, buf + offset, sizeof(struct inode));
    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs,
                      struct inode *inp,
                      int blockNum)
{
    typedef typeof(inp->i_addr[0]) addr_t;
    int naddr  = sizeof(inp->i_addr) / sizeof(inp->i_addr[0]);
    int perblk = DISKIMG_SECTOR_SIZE / sizeof(addr_t);
    char buf[DISKIMG_SECTOR_SIZE];

    // Small file: punteros directos
    if ((inp->i_mode & ILARG) == 0) {
        if (blockNum < naddr) return inp->i_addr[blockNum];
        return 0;
    }

    // Large file: indirectos simples en [0..naddr-2], doble indirecto en [naddr-1]
    int simple_limit = (naddr - 1) * perblk;
    if (blockNum < simple_limit) {
        int which  = blockNum / perblk;
        int offset = blockNum % perblk;
        addr_t blk = inp->i_addr[which];
        if (blk == 0) return 0;
        if (diskimg_readsector(fs->dfd, blk, buf) != DISKIMG_SECTOR_SIZE)
            return -1;
        addr_t *arr = (addr_t*)buf;
        return arr[offset];
    }

    // Doble indirecto
    int rem   = blockNum - simple_limit;
    int idx1  = rem / perblk;
    int idx2  = rem % perblk;

    addr_t dblblk = inp->i_addr[naddr - 1];
    if (dblblk == 0) return 0;
    if (diskimg_readsector(fs->dfd, dblblk, buf) != DISKIMG_SECTOR_SIZE)
        return -1;
    addr_t *first = (addr_t*)buf;

    addr_t blk2 = first[idx1];
    if (blk2 == 0) return 0;
    if (diskimg_readsector(fs->dfd, blk2, buf) != DISKIMG_SECTOR_SIZE)
        return -1;
    addr_t *second = (addr_t*)buf;

    return second[idx2];
}

int inode_getsize(struct inode *inp) {
    return (inp->i_size0 << 16) | inp->i_size1;
}
