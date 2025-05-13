#include <string.h>
#include "directory.h"
#include "inode.h"
#include "diskimg.h"   
#include "file.h"  
#include "unixfilesystem.h"
#include "direntv6.h"

int directory_findname(struct unixfilesystem *fs,
                       const char *name,
                       int dirinumber,
                       struct direntv6 *dirEnt)
{
    struct inode di;
    if (inode_iget(fs, dirinumber, &di) < 0) {
        return -1;
    }
    if ((di.i_mode & IFMT) != IFDIR) {
        return -1;
    }

    char buf[DISKIMG_SECTOR_SIZE];
    int blockno = 0;
    while (1) {
        int bytes = file_getblock(fs, dirinumber, blockno++, buf);
        if (bytes <= 0) break;

        int nents = bytes / sizeof(struct direntv6);
        struct direntv6 *ents = (struct direntv6*)buf;

        for (int i = 0; i < nents; i++) {
            if (ents[i].d_inumber != 0
             && strcmp(ents[i].d_name, name) == 0)
            {
                *dirEnt = ents[i];
                return ents[i].d_inumber;
            }
        }
    }

    return 0;
}
