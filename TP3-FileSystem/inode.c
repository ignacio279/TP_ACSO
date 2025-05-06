#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

#include <string.h>             // memcpy
#include "unixfilesystem.h"     // struct unixfilesystem, INODE_START_SECTOR
#include "direntv6.h"           // daddr_t, pero en este caso no se usa aquí


int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    // 1) Cantidad de inodos que caben en un sector
    int inodes_per_block = DISKIMG_SECTOR_SIZE / sizeof(struct inode);

    // 2) Número máximo de inodos en el FS
    int max_inodes = fs->superblock.s_isize * inodes_per_block;
    if (inumber < 1 || inumber > max_inodes) {
        // Inodo fuera de rango
        return -1;
    }

    // 3) Índice base‐0 y cálculo de sector + offset
    int idx    = inumber - 1;
    int sector = INODE_START_SECTOR + (idx / inodes_per_block);
    int offset = (idx % inodes_per_block) * sizeof(struct inode);

    // 4) Leer el sector completo
    char buf[DISKIMG_SECTOR_SIZE];
    int n = diskimg_blockread(fs->dfd, sector, buf);
    if (n != DISKIMG_SECTOR_SIZE) {
        // Error de lectura
        return -1;
    }

    // 5) Copiar sólo los bytes correspondientes al inodo
    memcpy(inp, buf + offset, sizeof(struct inode));
    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs,
    struct inode *inp,
    int blockNum)
{
// 1) Detectar small vs large
typedef typeof(inp->i_addr[0]) addr_t;  
int naddr   = sizeof(inp->i_addr) / sizeof(inp->i_addr[0]);
int perblk  = DISKIMG_SECTOR_SIZE / sizeof(addr_t);
char buf[DISKIMG_SECTOR_SIZE];

// Small file: punteros directos
if ((inp->i_mode & ILARG) == 0) {
if (blockNum < naddr)
return inp->i_addr[blockNum];
return 0;
}

// Large file: indirectos simples en [0..naddr-2], doble indirecto en [naddr-1]
int simple_limit = (naddr - 1) * perblk;
if (blockNum < simple_limit) {
int which  = blockNum / perblk;
int offset = blockNum % perblk;
addr_t blk = inp->i_addr[which];
if (blk == 0) return 0;
if (diskimg_blockread(fs->dfd, blk, buf) != DISKIMG_SECTOR_SIZE)
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
if (diskimg_blockread(fs->dfd, dblblk, buf) != DISKIMG_SECTOR_SIZE)
return -1;
addr_t *first = (addr_t*)buf;

addr_t blk2 = first[idx1];
if (blk2 == 0) return 0;
if (diskimg_blockread(fs->dfd, blk2, buf) != DISKIMG_SECTOR_SIZE)
return -1;
addr_t *second = (addr_t*)buf;

return second[idx2];
}


int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
