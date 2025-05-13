#include <string.h>
#include "diskimg.h"        
#include "inode.h"          
#include "file.h"           
#include "unixfilesystem.h" 

int file_getblock(struct unixfilesystem *fs,
                  int inumber,
                  int blockNo,
                  void *buf)
{
    struct inode inp;
    if (inode_iget(fs, inumber, &inp) < 0) {
        return -1;
    }

    int phys = inode_indexlookup(fs, &inp, blockNo);
    if (phys <= 0) {
        return 0;
    }

    int n = diskimg_readsector(fs->dfd, phys, buf);
    if (n < 0) {
        return -1;
    }

    int size     = inode_getsize(&inp);
    int fullBlks = size / DISKIMG_SECTOR_SIZE;
    if (blockNo < fullBlks) {
        return DISKIMG_SECTOR_SIZE;
    } else {
        int rem = size - fullBlks * DISKIMG_SECTOR_SIZE;
        return (rem > 0 ? rem : DISKIMG_SECTOR_SIZE);
    }
}
