#include <string.h>
#include "pathname.h"
#include "directory.h"
#include "unixfilesystem.h"
#include "direntv6.h"

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    // Si es la raíz, devolvemos directamente ROOT_INUMBER
    if (strcmp(pathname, "/") == 0) {
        return ROOT_INUMBER;  // normalmente 1
    }

    // Copiamos el path para tokenizar sin modificar el original
    char pathbuf[strlen(pathname) + 1];
    strcpy(pathbuf, pathname);

    // Empezamos en el inodo raíz
    int inum = ROOT_INUMBER;

    // Desmontamos componentes separados por '/'
    char *saveptr, *tok = strtok_r(pathbuf, "/", &saveptr);
    while (tok) {
        struct direntv6 dent;
        // directory_findname devuelve el inumber o 0 si no existe
        int next = directory_findname(fs, tok, inum, &dent);
        if (next <= 0) {
            return -1;  // componente no encontrado
        }
        inum = next;
        tok = strtok_r(NULL, "/", &saveptr);
    }

    return inum;
}
