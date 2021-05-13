/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#include "shell.h"

int c_delete_aux(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if(remove(fpath) < 0) {
        printf("Error %d removing %s: %s\n", errno, fpath, strerror(errno));
        return 1;
    }
    return 0;
}

void c_delete(cmd_args args) {
    if(!args.n) {
        list_flags = _C_LIST_FIRST | C_LIST_DIR;
        nftw(".", c_list_aux, 1020, FTW_PHYS | FTW_MOUNT | FTW_ACTIONRETVAL);
        return;
    }
    bool rec = !strncmp(args.args[0], "-rec", 5);
    if(!(args.n > (rec ? 1 : 0))) {
        list_flags = _C_LIST_FIRST | C_LIST_DIR;
        nftw(".", c_list_aux, 1020, FTW_PHYS | FTW_MOUNT | FTW_ACTIONRETVAL);
        return;
    }
    for(int i = rec ? 1 : 0; i < args.n; i++) {
        if(remove(args.args[i]) < 0) {
            if(errno == 39 && rec) {
                nftw(args.args[i], c_delete_aux, 1020, FTW_PHYS | FTW_MOUNT | FTW_DEPTH);
                return;
            } 
            printf("Error %d removing %s: %s\n", errno, args.args[i], strerror(errno));
        }
    }
}
