/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#include "proc_list.h"

static SEN sigstrnum[] = {
    {"HUP", SIGHUP},
    {"INT", SIGINT},
    {"QUIT", SIGQUIT},
    {"ILL", SIGILL},
    {"TRAP", SIGTRAP},
    {"ABRT", SIGABRT},
    {"IOT", SIGIOT},
    {"BUS", SIGBUS},
    {"FPE", SIGFPE},
    {"KILL", SIGKILL},
    {"USR1", SIGUSR1},
    {"SEGV",SIGSEGV},
    {"USR2", SIGUSR2},
    {"PIPE", SIGPIPE},
    {"ALRM", SIGALRM},
    {"TERM", SIGTERM},
    {"CHLD", SIGCHLD},
    {"CONT", SIGCONT},
    {"STOP", SIGSTOP},
    {"TSTP", SIGTSTP},
    {"TTIN", SIGTTIN},
    {"TTOU", SIGTTOU},
    {"URG", SIGURG},
    {"XCPU", SIGXCPU},
    {"XFSZ", SIGXFSZ},
    {"VTALRM", SIGVTALRM},
    {"PROF", SIGPROF},
    {"WINCH", SIGWINCH},
    {"IO", SIGIO},
    {"SYS", SIGSYS},
    //Optional signals
    #ifdef SIGPOLL
    {"POLL", SIGPOLL},
    #endif
    #ifdef SIGPWR
    {"PWR", SIGPWR},
    #endif
    #ifdef SIGEMT
    {"EMT", SIGEMT},
    #endif
    #ifdef SIGINFO
    {"INFO", SIGINFO},
    #endif
    #ifdef SIGSTKFLT
    {"STKFLT", SIGSTKFLT},
    #endif
    #ifdef SIGCLD
    {"CLD", SIGCLD},
    #endif
    #ifdef SIGLOST
    {"LOST", SIGLOST},
    #endif
    #ifdef SIGCANCEL
    {"CANCEL", SIGCANCEL},
    #endif
    #ifdef SIGTHAW
    {"THAW", SIGTHAW},
    #endif
    #ifdef SIGFREEZE
    {"FREEZE", SIGFREEZE},
    #endif
    #ifdef SIGLWP
    {"LWP", SIGLWP},
    #endif
    #ifdef SIGWAITING
    {"WAITING", SIGWAITING},
    #endif
    {NULL, -1}
};

int signum(char* sen) {
    if(!sen) return -1;
    int i;
    for (i = 0; sigstrnum[i].name != NULL; i++)
        if (!strcmp(sen, sigstrnum[i].name))
            return sigstrnum[i].sig;
    return -1;
}

char* signame(int sig) {
    int i;
    for (i = 0; sigstrnum[i].name != NULL; i++)
        if (sig == sigstrnum[i].sig)
            return sigstrnum[i].name;
    return "SIGUNKNOWN";
}

void proc_list_init(proc_list *list) {
    if(!list) return;
    list->first = NULL;
    list->last = NULL;
}

int proc_list_append(proc_list *list, proc_node element) {
    if(!list) return -3;
    if(!element.cmd) return -2;
    if(list->last) {
        list->last->next = malloc(sizeof(proc_node));
        if(!list->last->next) return -1;
        *(list->last->next) = element;
        list->last = list->last->next;
    } else {
        list->first = malloc(sizeof(proc_node));
        if(!list->first) return -1;
        *(list->first) = element;
        list->last = list->first;
    }
    return 0;
}

void proc_list_remove_exi(proc_list *list) {
    if(!list) return;
    proc_pos prev = NULL, it = list->first; int state, temp;
    errno = 0;
    while(it) {

        if(it->has_exited) {
            temp = it->pid;
            state = it->state;
        } else {
            temp = waitpid(it->pid, &state, WNOHANG | WUNTRACED | WCONTINUED);
            it->has_exited = temp && !WIFCONTINUED(state) && !WIFSTOPPED(state);
            if(state && temp) it->state = state;
            else {
                state = it->state;
                temp = it->state ? it->pid : 0;
            }
        }
        if (temp != it->pid && temp) {
            if(errno) perror("Unable to get process state"); 
            else printf("Unable to get process state: unknown error.\n");
            return;
        }

        if(WIFEXITED(state) && temp) {
            if(it == list->last) list->last = prev;
            if(prev) prev->next = it->next;
            else list->first = it->next;
            free(it->cmd);
            free(it);
        } else {
            prev = it;
        }
        it = prev ? prev->next : NULL;
    }
}

void proc_list_remove_sig(proc_list *list) {
    if(!list) return;
    proc_pos prev = NULL, it = list->first; int state, temp;
    errno = 0;
    while(it) {

        if(it->has_exited) {
            temp = it->pid;
            state = it->state;
        } else {
            temp = waitpid(it->pid, &state, WNOHANG | WUNTRACED | WCONTINUED);
            it->has_exited = temp && !WIFCONTINUED(state) && !WIFSTOPPED(state);
            if(state && temp) it->state = state;
            else {
                state = it->state;
                temp = it->state ? it->pid : 0;
            }
        }
        if (temp != it->pid && temp) {
            if(errno) perror("Unable to get process state"); 
            else printf("Unable to get process state: unknown error.\n");
            return;
        }

        if(WIFSIGNALED(state) && temp) {
            if(it == list->last) list->last = prev;
            if(prev) prev->next = it->next;
            else list->first = it->next;
            free(it->cmd);
            free(it);
        } else {
            prev = it;
        }
        it = prev ? prev->next : NULL;
    }
}

void proc_list_remove_node(proc_list* list, pid_t pid) {
    if(!list) return;
    proc_pos it = list->first; proc_pos prev = NULL;
    while(it && it->pid != pid) {
        prev = it;
        it = it->next;
    }
    if(it) {
        if(it == list->last) list->last = prev;
        if(prev) prev->next = it->next;
        else list->first = it->next;
        free(it->cmd);
        free(it);
    }
}

void proc_list_reset(proc_list *list) {
    if(!list) return;
    proc_pos temp, it = list->first;
    while(it) {
        temp = it->next;
        free(it->cmd);
        free(it);
        it = temp;
    }
    proc_list_init(list);
}

void proc_node_print(proc_pos node) {
    if(!node) return;
    int pr, state, temp;
    errno = 0;
    struct tm tm = *localtime(&node->time);
    if((pr = getpriority(PRIO_PROCESS, node->pid)) == -1 && errno) printf("%d: @-- '%s' %02d-%02d-%04d %02d:%02d", node->pid, node->cmd, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
    else printf("%d: @%.2d '%s' %02d-%02d-%04d %02d:%02d", node->pid, pr, node->cmd, tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
    
    if(node->has_exited) {
        temp = node->pid;
        state = node->state;
    } else {
        temp = waitpid(node->pid, &state, WNOHANG | WUNTRACED | WCONTINUED);
        node->has_exited = temp && !WIFCONTINUED(state) && !WIFSTOPPED(state);
        if(state && temp) node->state = state;
        else {
            state = node->state;
            temp = node->state ? node->pid : 0;
        }
    }

    if (temp != node->pid && temp) {
        if(errno) perror("Unable to get process state"); 
        else printf("Unable to get process state: unknown error.\n");
        return;
    }
    if(!temp) printf(" RUNNING");
    else if(WIFCONTINUED(state)) printf(" CONT");
    else if(WIFEXITED(state)) printf(" EXIT: %d", WEXITSTATUS(state));
    else if(WIFSIGNALED(state)) printf(" SIG: %s", signame(WTERMSIG(state)));
    else if(WIFSTOPPED(state)) printf(" STP: %s", signame(WSTOPSIG(state)));
    printf("\n");
}

void proc_list_print(proc_list list) {
    proc_pos it = list.first;
    if(!it) printf("No child processes running in the background\n");
    while(it) {
        proc_node_print(it);
        it = it->next;
    }
}

proc_pos proc_list_get(proc_list list, pid_t pid) {
    proc_pos it = list.first;
    while(it && it->pid != pid) it = it->next;
    return it;
}
