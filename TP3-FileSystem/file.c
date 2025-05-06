// file.c
#include <stdint.h>
#include <string.h>
#include "diskimg.h"         // diskimg_blockread, DISKIMG_SECTOR_SIZE
#include "inode.h"           // inode_indexlookup, inode_getsize
#include "file.h"            // declara file_getblock, forward de struct file
#include "unixfilesystem.h"  // struct unixfilesystem

// --- DEFINICIÓN PRIVADA de struct file, sólo en este .c ---
struct file {
    struct unixfilesystem *f_fs;  // opcional, según tu file_open
    struct inode          f_inode;
    // int32_t               f_pos;  // si querés llevar posición
};

// Ahora sí el compilador sabe qué es f->f_inode:
int file_getblock(struct unixfilesystem *fs,
                  struct file *f,
                  int32_t blockNum,
                  char *buf)
{
    // 1) Traducir bloque lógico → físico
    int phys = inode_indexlookup(fs, &f->f_inode, blockNum);
    if (phys <= 0) {
        // bloque fuera de rango o no asignado
        return 0;
    }

    // 2) Leer sector
    int n = diskimg_blockread(fs->dfd, phys, buf);
    if (n < 0) {
        // error I/O
        return -1;
    }

    // 3) Calcular bytes válidos
    int64_t size      = inode_getsize(&f->f_inode);
    int    fullBlks   = size / DISKIMG_SECTOR_SIZE;
    if (blockNum < fullBlks) {
        // bloque completo
        return DISKIMG_SECTOR_SIZE;
    } else {
        // bloque final (parcial o completo si exacto)
        int rem = size - fullBlks * DISKIMG_SECTOR_SIZE;
        return (rem > 0 ? rem : DISKIMG_SECTOR_SIZE);
    }
}
