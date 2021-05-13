/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#ifndef SHELL_OS
#define SHELL_OS

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define __USE_XOPEN_EXTENDED
#define __USE_GNU
#include <ftw.h>

#include "list.h"
#include "proc_list.h"

#define CMD_LENGTH 100
#define CMD_LIST_LENGTH 4096

#define H_GETPID 3875543937544491392U
#define H_GETPPID 9487481753249374786U
#define H_PWD 12603213319704018244U
#define H_CHDIR 3446495884711576933U
#define H_DATE 9380212855815724121U
#define H_TIME 17049455583112764388U
#define H_EXIT 1857265452769417861U
#define H_QUIT 16132469211670218550U
#define H_END 5458080918701766058U
#define H_AUTHORS 4258401178224887871U
#define H_HISTORIC 14573577509172390660U
#define H_CREATE 14242435953065679197U
#define H_DELETE 7400716028457665610U
#define H_LIST 15815063963508103297U
#define H_MEMORY 4210468128828289454U
    #define H_MEM_ALLOCATE 222688735898506607U
    #define H_MEM_DEALLOC 8454056507838259784U
    #define H_MEM_DELETEKEY 16772879731141420634U
    #define H_MEM_SHOW 5527325309519606353U
    #define H_MEM_SHOWVARS 4525183139299392338U
    #define H_MEM_SHOWFUNCS 13022649775729776291U
    #define H_MEM_PMAP 2356270220994763790U
#define H_RECURSE 10785248480216727618U
#define H_MEMDUMP 1191011000825132060U
#define H_MEMFILL 2488908740891753715U
#define H_READFILE 6866566126207745347U
#define H_WRITEFILE 15050039206727303626U
#define H_GETPRIORITY 2464994233165389567U
#define H_SETPRIORITY 3859160038206111019U
#define H_GETUID 2228595602451734043U
#define H_SETUID 13637369010024737487U
#define H_FORK 13884017689910363169U
#define H_EXECUTE 3886308616535162264U
#define H_FOREGROUND 9063553241596986816U
#define H_BACKGROUND 13457145975825750173U
#define H_RUNAS 7027605758001359469U
#define H_EXECUTEAS 2212599558535376891U
#define H_LISTPROCS 18369311579660872994U
#define H_PROC 3155262981684473825U
#define H_DELETEPROCS 10837872135345246447U

#define C_LIST_LONG 0b00000001
#define C_LIST_DIR 0b00000010
#define C_LIST_HID 0b00000100
#define C_LIST_REC 0b00001000
#define _C_LIST_FIRST 0b00010000

typedef struct cmd_args {
    int n;
    char** args;
} cmd_args;

typedef struct cmd_list {
    char* data[CMD_LIST_LENGTH];
    int last;
} cmd_list;

bool process_cmd(cmd_list*, char*, uint8_t);

int strpad_special(const char *);

void type_col(mode_t);

char type_char(mode_t);

char* mode_convert(mode_t);

int c_delete_aux(const char*, const struct stat*, int, struct FTW*);

int c_list_aux(const char*, const struct stat*, int, struct FTW*);

void c_delete(cmd_args);

void c_list(cmd_args);

extern uint8_t list_flags;

#endif