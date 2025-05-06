// file.c
#include <stdint.h>
#include <string.h>
#include "diskimg.h"
#include "inode.h"
#include "file.h"
#include "unixfilesystem.h"

// Tu definición privada de struct file
struct file {
    struct unixfilesystem *f_fs;
    struct inode          f_inode;
};

/**
 * Inicializa el objeto 'struct file' para que luego file_getblock
 * pueda usar f->f_inode.
 */
int file_open(struct unixfilesystem *fs,
              struct inode        *inp,
              struct file         *f)
{
    f->f_fs    = fs;
    f->f_inode = *inp;
    return 0;
}

/* ---------------- tu file_getblock va aquí, exacto como lo tienes ahora --------------- */

int file_getblock(struct unixfilesystem *fs,
                  struct file *f,
                  int32_t blockNum,
                  char *buf)
{
    // 1) Traducir bloque lógico → físico
    int phys = inode_indexlookup(fs, &f->f_inode, blockNum);
    if (phys <= 0) return 0;

    // 2) Leer sector
    int n = diskimg_blockread(fs->dfd, phys, buf);
    if (n < 0) return -1;

    // 3) Calcular bytes válidos
    int size     = inode_getsize(&f->f_inode);
    int fullBlks = size / DISKIMG_SECTOR_SIZE;
    if (blockNum < fullBlks) {
        return DISKIMG_SECTOR_SIZE;
    } else {
        int rem = size - fullBlks * DISKIMG_SECTOR_SIZE;
        return (rem > 0 ? rem : DISKIMG_SECTOR_SIZE);
    }
}
