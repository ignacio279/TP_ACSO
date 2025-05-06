#include <string.h>
#include "diskimg.h"         // diskimg_readsector, DISKIMG_SECTOR_SIZE
#include "inode.h"           // inode_iget, inode_indexlookup, inode_getsize
#include "file.h"            // declara file_getblock
#include "unixfilesystem.h"  // struct unixfilesystem

int file_getblock(struct unixfilesystem *fs,
                  int inumber,
                  int blockNo,
                  void *buf)
{
    struct inode inp;
    // 1) Carga el inodo
    if (inode_iget(fs, inumber, &inp) < 0) {
        return -1;
    }

    // 2) Traduce bloque lógico → físico
    int phys = inode_indexlookup(fs, &inp, blockNo);
    if (phys <= 0) {
        return 0;
    }

    // 3) Lee el sector completo
    int n = diskimg_readsector(fs->dfd, phys, buf);
    if (n < 0) {
        return -1;
    }

    // 4) Calcula cuántos bytes son válidos
    int size     = inode_getsize(&inp);
    int fullBlks = size / DISKIMG_SECTOR_SIZE;
    if (blockNo < fullBlks) {
        return DISKIMG_SECTOR_SIZE;
    } else {
        int rem = size - fullBlks * DISKIMG_SECTOR_SIZE;
        return (rem > 0 ? rem : DISKIMG_SECTOR_SIZE);
    }
}
