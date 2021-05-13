/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#include "list.h"

void mem_list_init(mem_list *list) {
    list->last = -1;
}

int mem_list_insert(mem_list *list, mem_info element) {
    if(list->last < MEM_LIST_LENGTH - 1) {
        mem_info* dest = malloc(sizeof(mem_info));
        if(!dest) return -1;
        list->last++;
        *dest = element;
        list->data[list->last] = dest;
        return list->last;
    }
    return -2;
}

int mem_list_remove(mem_list *list, int pos) {
    if(pos > list->last) return -1;
    if(list->data[pos]->type == MEM_TYPE_MAPPED) free(list->data[pos]->file_name);
    free(list->data[pos]);
    for(int i = pos; i < list->last; i++) list->data[pos] = list->data[pos + 1];
    list->last--;
    return 0;
}

void mem_list_reset(mem_list *list) {
    for(int i = list->last; i >= 0; i--) {
        if(list->data[i]->type == MEM_TYPE_MAPPED) free(list->data[i]->file_name);
        free(list->data[i]);
    }
    list->last = -1;
}

void mem_list_print(mem_list list, int type) {
    bool found = false;
    for(int i = 0; i <= list.last; i++) {
        if(type != MEM_TYPE_ALL && list.data[i]->type != type) continue;
        found = true;
        struct tm tm = *localtime(&(list.data[i]->time));
        switch (list.data[i]->type) {
        case MEM_TYPE_MALLOC:
            printf("\033[;32m");
            break;
        case MEM_TYPE_SHARED:
            printf("\033[;35m");
            break;
        case MEM_TYPE_MAPPED:
            printf("\033[;36m");
            break;
        }
        printf("%p [%8ldB] %02d-%02d-%04d %02d:%02d ", list.data[i]->addr, list.data[i]->size, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
        switch (list.data[i]->type) {
        case MEM_TYPE_MALLOC:
            printf("MALLOC\033[0m\n");
            break;
        case MEM_TYPE_SHARED:
            printf("SHARED K:%d\033[0m\n", list.data[i]->shm_key);
            break;
        case MEM_TYPE_MAPPED:
            printf("MAPPED FD:%d %s\033[0m\n", list.data[i]->file_descriptor, list.data[i]->file_name);
            break;
        }
    }
    if(!found) printf("No memory allocated so far\n");
}