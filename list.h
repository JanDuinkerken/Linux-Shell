/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#ifndef LIST_OS
#define LIST_OS

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#define MEM_LIST_LENGTH 4096

#define MEM_TYPE_ALL 0
#define MEM_TYPE_MALLOC 1
#define MEM_TYPE_SHARED 2
#define MEM_TYPE_MAPPED 3

typedef struct mem_info {
    void* addr;
    size_t size;
    time_t time;
    uint8_t type;
    char* file_name;
    int file_descriptor;
    key_t shm_key;
} mem_info;

typedef struct mem_list {
    mem_info* data[MEM_LIST_LENGTH];
    int last;
} mem_list;

void mem_list_init(mem_list *list);

int mem_list_insert(mem_list *list, mem_info element);

int mem_list_remove(mem_list *list, int pos);

void mem_list_reset(mem_list *list);

void mem_list_print(mem_list list, int type);

#endif