/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#include "shell.h"

void type_col(mode_t m) {
    switch (m & S_IFMT) {
        case S_IFSOCK:
            printf("\033[0;35m");
            break;
        case S_IFLNK:
            printf("\033[0;33m");
            break;;
        case S_IFREG:
            printf("\033[0;32m");
            break;
        case S_IFBLK:
            break;
        case S_IFDIR:
            printf("\033[0;36m");
            break;
        case S_IFCHR:
            break;
        case S_IFIFO:
            printf("\033[0;35m");
            break;
        default:
            printf("\033[0;31m");
            break;
    }
}

char type_char(mode_t m) {
    switch (m & S_IFMT) {          //Bitwise and with 0170000
        case S_IFSOCK: return 's'; //Socket
        case S_IFLNK:  return 'l'; //Symbolic link
        case S_IFREG:  return '-'; //File
        case S_IFBLK:  return 'b'; //Block device
        case S_IFDIR:  return 'd'; //Directory
        case S_IFCHR:  return 'c'; //Char device
        case S_IFIFO:  return 'p'; //Pipe
        default:       return '?'; //Unknown
    }
}

char* mode_convert(mode_t m) {
    char* permissions;
    permissions=(char*) malloc(12); //Remember to free this after use
    strcpy (permissions, "---------- ");
    permissions[0] = type_char(m);
    if (m & S_IRUSR) permissions[1] = 'r'; //Owner
    if (m & S_IWUSR) permissions[2] = 'w';
    if (m & S_IXUSR) permissions[3] = 'x';
    if (m & S_IRGRP) permissions[4] = 'r'; //Group
    if (m & S_IWGRP) permissions[5] = 'w';
    if (m & S_IXGRP) permissions[6] = 'x';
    if (m & S_IROTH) permissions[7] = 'r'; //Others
    if (m & S_IWOTH) permissions[8] = 'w';
    if (m & S_IXOTH) permissions[9] = 'x';
    if (m & S_ISUID) permissions[3] = 's'; //Setuid, setgid and stickybit
    if (m & S_ISGID) permissions[6] = 's';
    if (m & S_ISVTX) permissions[9] = 't';
    return (permissions);
}

int strpad_special(const char *str) {
    int len = 0;
    for(; *str != '\0'; str++) {
        if ((*str & 0xc0) == 0x80) len++;
    }
    return len;
}

uint8_t list_flags = 0; //000fRHDL
int c_list_aux(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    const char* name = strrchr(fpath, '/') + 1;
    if(name == (const char*)0x1) name = fpath;

    if((list_flags & _C_LIST_FIRST) && (list_flags & C_LIST_DIR) && (type_char(sb->st_mode) == 'd')) {
        //Does nothing. The brackets are here just for show.
    } else if(!(list_flags & C_LIST_HID) && *name == '.') {
        if(!(list_flags & _C_LIST_FIRST)) return FTW_SKIP_SUBTREE;
    } else if(list_flags & C_LIST_LONG) {
        struct tm time = *localtime(&(sb->st_mtime));
        char* flags = mode_convert(sb->st_mode);
        struct passwd *uid;
        char* uname;
        if((uid = getpwuid(sb->st_uid))) uname = uid->pw_name;
        else {
            char arr_uname[6];
            uname = arr_uname;
            snprintf(uname, 6, "%d", sb->st_uid);
        }
        struct group *gid;
        char* gname;
        if((gid = getgrgid(sb->st_gid))) gname = gid->gr_name;
        else {
            char arr_gname[6];
            gname = arr_gname;
            snprintf(gname, 6, "%d", sb->st_uid);
        }
        printf("%02d-%02d-%04d %02d:%02d  %-17ld %s %s %s %10ldB (%3lu) ", time.tm_mday, time.tm_mon + 1, time.tm_year + 1900, time.tm_hour, time.tm_min, sb->st_ino, uname, gname, flags, sb->st_size, sb->st_nlink);
        type_col(sb->st_mode);
        printf("%s", fpath);
        if(type_char(sb->st_mode) == 'l') {
            char buffer[100];
            buffer[99] = '\0';
            readlink(fpath, buffer, 99);
            printf(" → %s", buffer);
        }
        printf("\033[0m\n");
        free(flags);
    } else {
        type_col(sb->st_mode);
        printf("%-*s\033[0m │ %10ldB\n", 50 + strpad_special(fpath), fpath, sb->st_size);
    }

    if(!(list_flags & C_LIST_DIR) || (!(list_flags & C_LIST_REC) && !(list_flags & _C_LIST_FIRST))) {
        list_flags &= ~_C_LIST_FIRST; //Clear 'first' bit
        return FTW_SKIP_SUBTREE;
    } else {
        list_flags &= ~_C_LIST_FIRST; //Clear 'first' bit
        return FTW_CONTINUE;
    }
}

void c_list(cmd_args args) {
    list_flags = _C_LIST_FIRST;
    int i = 0;
    for(; i < args.n; i++) {
             if(!strncmp(args.args[i], "-long", 6)) list_flags |= C_LIST_LONG;
        else if(!strncmp(args.args[i],  "-dir", 5)) list_flags |= C_LIST_DIR;
        else if(!strncmp(args.args[i],  "-hid", 5)) list_flags |= C_LIST_HID;
        else if(!strncmp(args.args[i],  "-rec", 5)) list_flags |= C_LIST_REC;
        else break;
    }
    if(!(args.n > i)) {
        list_flags |= C_LIST_DIR; //dir
        nftw(".", c_list_aux, 1020, FTW_PHYS | FTW_MOUNT | FTW_ACTIONRETVAL);
        return;
    }
    for(; i < args.n; i++) {
        list_flags |= _C_LIST_FIRST;
        nftw(args.args[i], c_list_aux, 1020, FTW_PHYS | FTW_MOUNT | FTW_ACTIONRETVAL);
        printf("\n");
    }
}
