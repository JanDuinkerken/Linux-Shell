/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#ifndef PROC_LIST_OS
#define PROC_LIST_OS

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

typedef struct SEN {
    char* name;
    int sig;
} SEN;

typedef struct proc_node {
    struct proc_node * next;
    pid_t pid;
    time_t time;
    char* cmd;
    int state;
    bool has_exited;
} proc_node;

typedef proc_node * proc_pos;

typedef struct proc_list {
    proc_pos first;
    proc_pos last;
} proc_list;

void proc_list_init(proc_list *list);

int proc_list_append(proc_list *list, proc_node element);

void proc_list_remove_exi(proc_list *list);

void proc_list_remove_sig(proc_list *list);

void proc_list_remove_node(proc_list* list, pid_t pid);

void proc_list_reset(proc_list *list);

void proc_node_print(proc_pos node);

void proc_list_print(proc_list list);

proc_pos proc_list_get(proc_list list, pid_t pid);

#endif