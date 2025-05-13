#include <string.h>
#include "pathname.h"
#include "directory.h"
#include "unixfilesystem.h"
#include "direntv6.h"

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (strcmp(pathname, "/") == 0) {
        return ROOT_INUMBER;  
    }

    char pathbuf[strlen(pathname) + 1];
    strcpy(pathbuf, pathname);

    int inum = ROOT_INUMBER;

    char *saveptr, *tok = strtok_r(pathbuf, "/", &saveptr);
    while (tok) {
        struct direntv6 dent;
        int next = directory_findname(fs, tok, inum, &dent);
        if (next <= 0) {
            return -1;  
        }
        inum = next;
        tok = strtok_r(NULL, "/", &saveptr);
    }

    return inum;
}
