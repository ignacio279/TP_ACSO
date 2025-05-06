#include <string.h>
#include "directory.h"
#include "inode.h"
#include "diskimg.h"   // para DISKIMG_SECTOR_SIZE
#include "file.h"      // para file_getblock
#include "unixfilesystem.h"
#include "direntv6.h"

int directory_findname(struct unixfilesystem *fs,
                       const char *name,
                       int dirinumber,
                       struct direntv6 *dirEnt)
{
    struct inode di;
    // 1) Leer el inodo del directorio
    if (inode_iget(fs, dirinumber, &di) < 0) {
        return -1;
    }
    // 2) Asegurarnos de que sea realmente un directorio
    if ((di.i_mode & IFMT) != IFDIR) {
        return -1;
    }

    // 3) Recorremos bloque a bloque
    char buf[DISKIMG_SECTOR_SIZE];
    int blockno = 0;
    while (1) {
        int bytes = file_getblock(fs, dirinumber, blockno++, buf);
        if (bytes <= 0) break;

        // 4) Cada buf contiene un array de direntv6
        int nents = bytes / sizeof(struct direntv6);
        struct direntv6 *ents = (struct direntv6*)buf;

        // 5) Buscamos el nombre
        for (int i = 0; i < nents; i++) {
            if (ents[i].d_inumber != 0
             && strcmp(ents[i].d_name, name) == 0)
            {
                // 6) Al encontrarlo, copiamos la entrada y devolvemos su inumber
                *dirEnt = ents[i];
                return ents[i].d_inumber;
            }
        }
    }

    // 7) Si no existe, devolvemos 0
    return 0;
}
