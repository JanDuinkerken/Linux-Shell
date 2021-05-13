/*
Jan Duinkerken Rodríguez | Jesús Mosteiro García
jan.duinkerken@udc.es    | jesus.mosteiro@udc.es
*/

#include "shell.h"

static mem_list mem_data;
static proc_list proc_data;

bool read_input(char* buffer, int n) {
    int i = 0; char c; n--;
    while((c = getchar()) != '\n') {
        if(c == EOF) return true;
        if(i < n) {
            buffer[i] = c;
            i++;
        }
    }
    buffer[i] = '\0';
    return false;
}

void cmd_list_init(cmd_list *list) {
    list->last = -1;
}

int cmd_list_insert(cmd_list *list, char* element) {
    unsigned int l = strlen(element) + 1;
    if(l > CMD_LENGTH) return -2;
    char* dest = malloc(l);
    if(!dest) return -1;
    if(list->last < CMD_LIST_LENGTH - 1) list->last++;
    strncpy(dest, element, l);
    for(int i = list->last; i > 0; i--) {
        if(i == CMD_LIST_LENGTH - 1) free(list->data[i]);
        list->data[i] = list->data[i - 1];
    }
    list->data[0] = dest;
    return 0;
}

void cmd_list_reset(cmd_list * list) {
    for(int i = list->last; i >= 0; i--) free(list->data[i]);
    list->last = -1;
}

static cmd_list *free_on_interrupt = NULL;
void sig_handler() {
    if(free_on_interrupt) cmd_list_reset(free_on_interrupt);
    mem_list_reset(&mem_data);
    proc_list_reset(&proc_data);
    printf("\nInterrupt signal received. Exiting...\n\n");
    exit(1);
}

//Implements FNV hash function, variant 1a. Returns hash and splits parameters from the input.
uint64_t hash(unsigned char *str, uint8_t i, char **params) {
    uint64_t hash = 0x811c9dc5;
    while (*str && *str != ' ' && *str != '\t' && *str != '\n') {
        hash ^= (unsigned char) *str++;
        hash *= 0x01000193;
    }
    if(params) {
        if(*str == ' ' || *str == '\t') *params = (char*)(str + 1);
        else *params = NULL;
    }
    *str = '\0';
    return hash;
}

void param_to_args(char* params, cmd_args* args) {
    args->n = 0; args->args = NULL;
    if(params) {
        args->n++;
        for(int i = 0; i < CMD_LENGTH && params[i] != '\0'; i++) {
            if(params[i] == ' ' || params[i] == '\t') args->n++;
            if(params[i] == '\n') params[i] = '\0';
        }
        args->args = malloc((args->n + 1) * sizeof(char*));
        args->args[args->n] = NULL;
        if(!args->args) {
            if(free_on_interrupt) cmd_list_reset(free_on_interrupt);
            printf("\nOut of heap space. Exiting...\n\n");
            exit(1);
        }
        for(int i = 0; i < args->n; i++) {
            args->args[i] = strtok(params, " \t");
            if(!args->args[i]) {
                args->n--;
                i--;
            } else params = NULL;
        }
    }
}

void c_getpid() {
    printf("%d\n", getpid());
}

void c_getppid() {
    printf("%d\n", getppid());
}

void c_pwd() {
    char str[50];
    getcwd(str, 50);
    printf("%s\n", str);
}

void c_chdir(cmd_args location) {
    if(!location.n) {
        c_pwd();
        return;
    }
    if(chdir(location.args[0])) perror("Error changing directory");
}

void c_date() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%02d/%02d/%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

void c_time() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("%02d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void c_authors(cmd_args args) {
    char names[] = "Jan Duinkerken Rodríguez | Jesús Mosteiro García\n";
    char logins[]= "jan.duinkerken@udc.es    | jesus.mosteiro@udc.es\n";
    for(int i = 0; i < args.n; i++) {
        if(!strcmp(args.args[i], "-n")) logins[0] = '\0';
        else if(!strcmp(args.args[i], "-l")) names[0] = '\0';
        else {
            printf("Invalid argument: %s\n", args.args[i]);
            return;
        }
    }
    printf("%s%s", names, logins);
}

void c_historic(cmd_args args, cmd_list *history, char * in) {
    unsigned int n = 0;
    if(!args.n) {
        printf("Command history:\n");
        for(int i = history->last; i >= 0; i--) printf("%d: %s\n", i + 1, history->data[i]);
        return;
    }
    for(int i = 0; i < args.n; i++) {
        if(!strncmp(args.args[i], "-c", 3)) {
            cmd_list_reset(history);
            return;
        }
        if(!strncmp(args.args[i], "-r", 2) && sscanf(args.args[i], "-r%u", &n)) {
        	if(n <= history->last) {
        	    strncpy(in, history->data[n], CMD_LENGTH);
        	    printf("Executing command \033[1;34m→\033[0m  %s\n", in);
        	    process_cmd(history, in, 1);
        	} else printf("Index %d out of bounds\n", n);
        	return;
        }
        if(!strncmp(args.args[i], "-", 1) && sscanf(args.args[i], "-%u", &n)) {
            printf("Command history:\n");
    	    n--;
            for(int i = n > history->last ? history->last : n; i >= 0; i--) printf("%d: %s\n", i + 1, history->data[i]);
            return;
        }
        printf("Invalid argument: %s\n", args.args[i]);
        return;
    }
}

void c_create(cmd_args args) {
    if(!args.n) {
        list_flags = _C_LIST_FIRST | C_LIST_DIR;
        nftw(".", c_list_aux, 50, FTW_PHYS | FTW_MOUNT | FTW_ACTIONRETVAL);
        return;
    }
    bool dir = !strncmp(args.args[0], "-dir", 5);
    if(!(args.n > (dir ? 1 : 0))) {
        list_flags = _C_LIST_FIRST | C_LIST_DIR;
        nftw(".", c_list_aux, 50, FTW_PHYS | FTW_MOUNT | FTW_ACTIONRETVAL);
        return;
    }
    if(dir) {
        if(mkdir(args.args[1], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) perror("Error creating directory");
    } else {
        if(creat(args.args[0], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) < 0) {
            perror("Error creating file");
        }
    }
}

void * get_shm(key_t key, size_t size) {
    void * p; int aux, id, flags = 0777; struct shmid_ds s;
    if(size) flags = flags | IPC_CREAT | IPC_EXCL; //Create new
    if(key == IPC_PRIVATE) {
        errno = EINVAL;
        return NULL;
    }
    if((id = shmget(key, size, flags)) == -1) return NULL;
    if((p = shmat(id, NULL, 0)) == (void *) - 1) {
        aux = errno;
        if(size) shmctl(id, IPC_RMID, NULL); //Remove
        errno = aux;
        return NULL;
    }
    shmctl(id, IPC_STAT, & s);

    mem_info list_data;
    list_data.addr = p;
    list_data.size = s.shm_segsz;
    list_data.time = time(NULL);
    list_data.type = MEM_TYPE_SHARED;
    list_data.shm_key = key;
    mem_list_insert(&mem_data, list_data);

    return p;
}

void c_alloc_create_sh(cmd_args args) {
    //arg[0]: key | arg[1]: size
    key_t k; size_t size = 0; void * p;
    if(args.n < 2) {
        mem_list_print(mem_data, MEM_TYPE_SHARED);
        return;
    }
    k = (key_t) atoi(args.args[0]);
    size = (size_t)atoll(args.args[1]);
    if(!(p = get_shm(k, size))) perror("Unable to get shared memory");
    else printf("Shared memory with key %d allocated at %p\n", k, p);
}

void c_alloc_sh(cmd_args args) {
    //arg[0]: key | arg[1]: size
    key_t k; void * p;
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_SHARED);
        return;
    }
    k = (key_t) atoi(args.args[0]);
    if(!(p = get_shm(k, 0))) perror("Unable to get shared memory");
    else printf("Shared memory with key %d allocated at %p\n", k, p);
}

void * mmap_file(char * file, int protection) {
    int df, map = MAP_PRIVATE, mode = O_RDONLY; struct stat s; void * p;
    if(protection & PROT_WRITE) mode = O_RDWR;
    if(stat(file, & s) == -1 || (df = open(file, mode)) == -1) return NULL;
    if((p = mmap(NULL, s.st_size, protection, map, df, 0)) == MAP_FAILED) return NULL;

    mem_info list_data;
    list_data.addr = p;
    list_data.size = s.st_size;
    list_data.time = time(NULL);
    list_data.type = MEM_TYPE_MAPPED;
    list_data.file_descriptor = df;
    size_t l = strlen(file) + 1;
    if(l > 2048) l = 2048;
    list_data.file_name = malloc(l);
    strncpy(list_data.file_name, file, l);
    list_data.file_name[l - 1] = '\0';
    mem_list_insert(&mem_data, list_data);

    return p;
}

void c_alloc_mmap(cmd_args args) {
    //arg[0]: file name | arg[1]: permissions
    void * p; int protection = 0;
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_MAPPED);
        return;
    }
    if(args.n > 1 && strlen(args.args[1]) < 4) {
        if(strchr(args.args[1], 'r') != NULL) protection |= PROT_READ;
        if(strchr(args.args[1], 'w') != NULL) protection |= PROT_WRITE;
        if(strchr(args.args[1], 'x') != NULL) protection |= PROT_EXEC;
    }
    if(!(p = mmap_file(args.args[0], protection))) perror("Unable to map file");
    else printf("File %s mapped at %p\n", args.args[0], p);
}

void c_delete_key(cmd_args args) {
    //arg[0]: key as string
    key_t k; int id;
    if(!args.n || (k = (key_t)strtoul(args.args[0], NULL, 10)) == IPC_PRIVATE) {
        printf("Invalid key\n");
        return;
    }
    if((id = shmget(k, 0, 0666)) == -1) {
        perror("Unable to get shared memory");
        return;
    }
    if(shmctl(id, IPC_RMID, NULL) == -1) perror("Unable to remove shared memory\n");
}

void c_alloc_malloc(cmd_args args) {
    void * p; size_t size = 0;
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_MALLOC);
        return;
    }
    if(!(size = (size_t)atoll(args.args[0]))){
        printf("Invalid size\n");
        return;
    }
    if(!(p = malloc(size))) {
        perror("Unable to allocate memory");
        return;
    }

    mem_info list_data;
    list_data.addr = p;
    list_data.size = size;
    list_data.time = time(NULL);
    list_data.type = MEM_TYPE_MALLOC;
    mem_list_insert(&mem_data, list_data);

    printf("Allocated %ld bytes at %p\n", size, p);
}

void c_alloc(cmd_args args) {
    if(!args.n) {
        printf("Allocation type required\n");
        return;
    }
    char* type = args.args[0];
    args.n--;
    args.args = &args.args[1];
    if(!strcmp(type, "-malloc")) {
        c_alloc_malloc(args);
    } else if(!strcmp(type, "-mmap")) {
        c_alloc_mmap(args);
    } else if(!strcmp(type, "-createshared")) {
        c_alloc_create_sh(args);
    } else if(!strcmp(type, "-shared")) {
        c_alloc_sh(args);
    } else printf("Invalid allocation type\n");
}

void dealloc_malloc(int pos) {
    free(mem_data.data[pos]->addr);
    mem_list_remove(&mem_data, pos);
}

void dealloc_mmap(int pos) {
    if(munmap(mem_data.data[pos]->addr, mem_data.data[pos]->size) == -1) perror("Unable to remove mapped memory");
    else {
        close(mem_data.data[pos]->file_descriptor);
        mem_list_remove(&mem_data, pos);
    }
}

void dealloc_sh(int pos) {
    int id;
    if((id = shmget(mem_data.data[pos]->shm_key, 0, 0666)) == -1) {
        perror("Unable to get shared memory");
        return;
    }
    if(shmctl(id, IPC_RMID, NULL) == -1) perror("Unable to remove shared memory");
    else {
        shmdt(mem_data.data[pos]->addr);
        mem_list_remove(&mem_data, pos);
    }
}

void c_dealloc_malloc(cmd_args args) {
    size_t size;
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_MALLOC);
        return;
    }
    if(!(size = (size_t)atoll(args.args[0]))){
        printf("Invalid size\n");
        return;
    }
    bool found = false;
    for(int i = 0; i <= mem_data.last; i++) {
        if(mem_data.data[i]->type == MEM_TYPE_MALLOC && mem_data.data[i]->size == size) {
            dealloc_malloc(i);
            found = true;
            break;
        }
    }
    if(!found) printf("Couldn't find malloc with size %lu\n", size);
}

void c_dealloc_mmap(cmd_args args) {
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_MAPPED);
        return;
    }
    bool found = false;
    for(int i = 0; i <= mem_data.last; i++) {
        if(mem_data.data[i]->type == MEM_TYPE_MAPPED && !strcmp(args.args[0], mem_data.data[i]->file_name)) {
            dealloc_mmap(i);
            found = true;
            break;
        }
    }
    if(!found) printf("Couldn't find mmap with file %s\n", args.args[0]);
}

void c_dealloc_sh(cmd_args args) {
    key_t k;
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_SHARED);
        return;
    }
    if(!args.n || (k = (key_t)strtoul(args.args[0], NULL, 10)) == IPC_PRIVATE) {
        printf("Invalid key\n");
        return;
    }
    bool found = false;
    for(int i = 0; i <= mem_data.last; i++) {
        if(mem_data.data[i]->type == MEM_TYPE_SHARED && k == mem_data.data[i]->shm_key) {
            dealloc_sh(i);
            found = true;
            break;
        }
    }
    if(!found) printf("Couldn't find shared memory with key %d\n", k);
}

void c_dealloc(cmd_args args) {
    void* p;
    if(!args.n) {
        printf("Allocation type required\n");
        return;
    }
    char* type = args.args[0];
    args.n--;
    args.args = &args.args[1];
    if(!strcmp(type, "-malloc")) {
        c_dealloc_malloc(args);
    } else if(!strcmp(type, "-mmap")) {
        c_dealloc_mmap(args);
    } else if(!strcmp(type, "-shared")) {
        c_dealloc_sh(args);
    } else if((p = (void*)strtoul(type, NULL, 0))) {
        bool found = false;
        for(int i = 0; i <= mem_data.last; i++) {
            if(mem_data.data[i]->addr == p) {
                switch (mem_data.data[i]->type) {
                case MEM_TYPE_MALLOC:
                    dealloc_malloc(i);
                    break;
                case MEM_TYPE_MAPPED:
                    dealloc_mmap(i);
                    break;
                case MEM_TYPE_SHARED:
                    dealloc_sh(i);
                    break;
                }
                found = true;
                break;
            }
        }
        if(!found) printf("Couldn't find block with address %p\n", p);
    } else printf("Invalid allocation type\n");
}

void c_dopmap() {
    pid_t pid; char thepid[32]; char * argv[3] = {"pmap", thepid, NULL};
    sprintf(thepid, "%d", (int)getpid());
    if ((pid = fork()) == -1) {
        perror("Unable to create process");
        return;
    }
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) perror("Cannot execute pmap");
        exit(1);
    }
    waitpid(pid, NULL, 0);
}

void c_show_vars() {
    int three, local, variables;
    printf("Extern variables:\n\tmem_data: %p\n\tfree_on_interrupt: %p\n\tlist_flags: %p\nAutomatic variables:\n\tthree: %p\n\tlocal: %p\n\tvariables: %p\n", &mem_data, &free_on_interrupt, &list_flags, &three, &local, &variables);
}

void c_show_funcs(bool library) {
    printf("Program functions:\n\thash: %p\n\tsig_handler: %p\n\tread_input: %p\n", &hash, &sig_handler, &read_input);
    if(library) printf("Library functions:\n\tmalloc: %p\n\tfree: %p\n\tprintf: %p\n", &malloc, &free, &printf);
}

void c_show(cmd_args args) {
    int flag;
    if(!args.n) {
        c_show_vars();
        c_show_funcs(false);
        return;
    }
    if(!strcmp(args.args[0], "-malloc")) flag = MEM_TYPE_MALLOC;
    else if(!strcmp(args.args[0], "-mmap")) flag = MEM_TYPE_MAPPED;
    else if(!strcmp(args.args[0], "-shared")) flag = MEM_TYPE_SHARED;
    else if(!strcmp(args.args[0], "-all")) flag = MEM_TYPE_ALL;
    else {
        printf("Unrecognized option '%s'\n", args.args[0]);
        return;
    }
    mem_list_print(mem_data, flag);
}

void c_memory(cmd_args args) {
    if(!args.n) {
        mem_list_print(mem_data, MEM_TYPE_ALL);
        return;
    }
    char* type = args.args[0];
    uint64_t h = hash((unsigned char *) args.args[0], CMD_LENGTH, NULL);
    args.n--;
    args.args = &args.args[1];
    switch (h) {
    case H_MEM_ALLOCATE:
        c_alloc(args);
        break;
    case H_MEM_DEALLOC:
        c_dealloc(args);
        break;
    case H_MEM_DELETEKEY:
        c_delete_key(args);
        break;
    case H_MEM_SHOW:
        c_show(args);
        break;
    case H_MEM_SHOWVARS:
        c_show_vars();
        break;
    case H_MEM_SHOWFUNCS:
        c_show_funcs(true);
        break;
    case H_MEM_PMAP:
        c_dopmap();
        break;
    default:
        printf("Unrecognized option '%s'\n", type);
        break;
    }
}

void recursive(int n) {
    char automatic_arr[MEM_LIST_LENGTH];
    static char static_arr[MEM_LIST_LENGTH];
    printf ("Parameter n: %d at %p\n", n, &n);
    printf ("Static array at: %p\n", static_arr);
    printf ("Automatic array at: %p\n", automatic_arr);
    if (--n > 0) recursive(n);
}

void c_recurse(cmd_args args) {
    if(!args.n) {
        printf("Parameter n is required\n");
        return;
    }
    int n = atoi(args.args[0]);
    if(n > 0) recursive(atoi(args.args[0]));
    else printf("Invalid parameter n\n");
}

#define READ_ALL ((ssize_t)-1)
ssize_t read_file(char * file, void * p, ssize_t n) {
    ssize_t nread, size = n; int df, aux; struct stat s;
    if(stat(file, &s) == -1 || (df = open(file, O_RDONLY)) == -1) return READ_ALL;
    if(n == READ_ALL) size = (ssize_t)s.st_size;
    if((nread = read(df, p, size)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return READ_ALL;
    }
    close(df);
    return nread;
}

void c_readfile(cmd_args args) {
    ssize_t size; void* p;
    if(args.n < 2) {
        printf("Parameters fich and addr are required\n");
        return;
    }
    if(args.n < 3 || !(size = atoll(args.args[2]))) size = READ_ALL;
    if(!(p = (void*)strtoul(args.args[1], NULL, 0))) printf("Invalid destination address\n");
    else if(read_file(args.args[0], p, size) == READ_ALL) perror("Error reading file");
}

int write_file(char * file, void * p, ssize_t n) {
    int df, aux;
    if((df = open(file, O_WRONLY | O_CREAT)) == -1) return -1;
    if((write(df, p, n)) == -1) {
        aux = errno;
        close(df);
        errno = aux;
        return -1;
    }
    close(df);
    return 0;
}

void c_writefile(cmd_args args) {
    bool ow = false; ssize_t size; void* p; struct stat s;
    if(!args.n) {
        printf("Parameters fich, addr and cont are required\n");
        return;
    }
    if(!strcmp(args.args[0], "-o")) {
        args.n--;
        args.args = &args.args[1];
        ow = true;
    }
    if(args.n < 3) {
        printf("Parameters fich, addr and cont are required\n");
        return;
    }
    size = atoll(args.args[2]);
    if(!(p = (void*)strtoul(args.args[1], NULL, 0))) printf("Invalid destination address\n");
    else if(stat(args.args[0], &s) == -1) {
        if(write_file(args.args[0], p, size) == -1) perror("Unable to create file");
    } else if(ow) {
        if(remove(args.args[0]) == -1) perror("Unable to overwrite file");
        else if(write_file(args.args[0], p, size) == -1) perror("Unable to create file");
    } else printf("File already exists\n");
}

void c_memdump(cmd_args args) {
    ssize_t size; char* p; ssize_t i;
    if(!args.n) {
        printf("Parameter addr is required\n");
        return;
    }
    if(args.n < 2 || !(size = atoll(args.args[1]))) size = 25;
    if(!(p = (char*)strtoul(args.args[0], NULL, 0))) printf("Invalid destination address\n");
    else {
        for(i = 0; i < size * 2; i++) {
            int temp = (size * 2 / 50 + 1) * 50 - i <= 50 ? (size * 2) % 50 : 50;
            if(i % 50 > (temp / 2) - 1) printf("0x%02x ", p[i - 25 * (i / 50 + 1)] & 0xFF);
            else {
                char c = p[i - 25 * (i / 50)] & 0xFF;
                if(c & 0x80) c = ' ';
                switch(c) {
                    case '\0':
                        printf("'\\0' ");
                        break;
                    case '\n':
                        printf("'\\n' ");
                        break;
                    case '\t':
                        printf("'\\t' ");
                        break;
                    case '\r':
                        printf("'\\r' ");
                        break;
                    default:
                        printf(" '%c' ", c);
                        break;
                }
            }
            if(i % 25 == (temp / 2) - 1) printf("\n");
        }
        if(i % 25) printf("\n");
    }
}

void c_memfill(cmd_args args) {
    ssize_t size; char c; char* p;
    if(!args.n) {
        printf("Parameter addr is required\n");
        return;
    }
    if(args.n < 2 || !(size = atoll(args.args[1]))) size = 25;
    if(args.n > 2 && strlen(args.args[2]) == 1) c = *args.args[2];
    else if(args.n < 3 || !(c = strtoul(args.args[2], NULL, 0))) c = 'A';
    if(!(p = (char*)strtoul(args.args[0], NULL, 0))) printf("Invalid destination address\n");
    else for(ssize_t i = 0; i < size; i++) p[i] = c;
}

uid_t useruid(char * name) {
    struct passwd * p;
    if ((p = getpwnam(name)) == NULL)
        return (uid_t)-1;
    return p -> pw_uid;
}

char* username(uid_t uid) {
    struct passwd * p;
    if ((p = getpwuid(uid)) == NULL) return ("????");
    return p -> pw_name;
}

void c_getuid() {
    uid_t real = getuid(), efec = geteuid();
    printf("Real credentials: %d, (%s)\n", real, username(real));
    printf("Effective credentials: %d, (%s)\n", efec, username(efec));
}

void c_setuid(cmd_args args) {
    uid_t uid;
    int u;
    if (!args.n || (!strcmp(args.args[0], "-l") && args.n < 2)) {
        c_getuid(args);
        return;
    }
    if (!strcmp(args.args[0], "-l")) {
        if ((uid = useruid(args.args[1])) == (uid_t)-1) {
            printf("User %s not found\n", args.args[1]);
            return;
        }
    } else if ((uid = (uid_t)((u = atoi(args.args[0])) < 0) ? -1 : u) == (uid_t)-1) {
        printf("Not a valid value for credential %s\n", args.args[1]);
        return;
    }
    if (seteuid(uid) == -1) perror("Impossible to change credential"); //Changed setuid for seteuid. Sets the EFFECTIVE credentials, as stated on the assignment PDF.
}

void c_getpriority(cmd_args args) {
    int pr;
    errno = 0;
    if(!args.n) {
        if((pr = getpriority(PRIO_PROCESS, getpid())) == -1 && errno) perror("Unable to get priority"); 
        else printf("%d\n", pr);
        return;
    }
    if((pr = getpriority(PRIO_PROCESS, atoi(args.args[0]))) == -1 && errno) perror("Unable to get priority"); 
    else printf("%d\n", pr);
}

void c_setpriority(cmd_args args) {
    int pr, temp, temp2;
    errno = 0;
    if(!args.n) {
        if((pr = getpriority(PRIO_PROCESS, getpid())) == -1) perror("Unable to get priority");
        else printf("%d\n", pr);
        return;
    }
    if(args.n < 2) {
        temp = atoi(args.args[0]);
        if(setpriority(PRIO_PROCESS, getpid(), temp) == -1) perror("Unable to set priority");
        else {
            if((pr = getpriority(PRIO_PROCESS, getpid())) == -1 && errno) perror("Unable to check whether priority was set");
            else if(pr != temp) printf("Warning: Priority was set to %d. Was this the intended result?\n", pr);
        }
        return;
    }
    temp = atoi(args.args[1]);
    temp2 = atoi(args.args[0]);
    if(setpriority(PRIO_PROCESS, temp2, temp) == -1) perror("Unable to set priority"); 
    else {
        if((pr = getpriority(PRIO_PROCESS, temp2)) == -1 && errno) perror("Unable to check whether priority was set");
        else if(pr != temp) printf("Warning: Priority was set to %d. Was this the intended result?\n", pr);
    }
}

void c_fork() {
    int knight, status;
    if((knight = fork()) == -1) {
        perror("Unable to fork");
        return;
    } else if (!knight) {
        //Child process
        printf("Child process started\n");
    } else {
        //Parent process
        printf("Forked with PID %d\n", knight);
        if(waitpid(knight, &status, 0) == -1) perror("Unable to wait");
        printf("Child process finished with status %d\n\n", status);
    }
}

void c_execute(cmd_args args) {
    if(!args.n) {
        printf("Parameter prog is required\n");
        return;
    }
    if(args.n > 1 && args.args[args.n - 1][0] == '@') {
        if(setpriority(PRIO_PROCESS, getpid(), atoi(&args.args[args.n - 1][1])) == -1) {
            perror("Unable to set priority"); 
            return;
        }
        args.n--;
        args.args[args.n] = NULL;
    }
    if(execvp(args.args[0], args.args) == -1) printf("Unable to execute %s: %s\n", args.args[0], strerror(errno));
}

void c_foreground(cmd_args args) {
    int knight;
    if(!args.n) {
        printf("Parameter prog is required\n");
        return;
    }
    if((knight = fork()) == -1) {
        perror("Unable to fork");
        return;
    } else if (!knight) {
        //Child process
        if(args.n > 1 && args.args[args.n - 1][0] == '@') {
            if(setpriority(PRIO_PROCESS, getpid(), atoi(&args.args[args.n - 1][1])) == -1) {
                perror("Unable to set priority"); 
                exit(255);
            }
            args.n--;
            args.args[args.n] = NULL;
        }
        if(execvp(args.args[0], args.args) == -1) printf("Unable to execute %s: %s\n", args.args[0], strerror(errno));
        exit(255);
    } else {
        //Parent process
        if(waitpid(knight, NULL, 0) == -1) perror("Unable to wait");
    }
}

void new_bg_process(char* pog, cmd_args args, uid_t uid) {
    pid_t knight; proc_node node; size_t siz, temp; size_t * temp2; char * spc;
    if((knight = fork()) == -1) {
        perror("Unable to fork");
        return;
    } else if (!knight) {
        //Child process
        if (uid != -1 && seteuid(uid) == -1) {
            perror("Impossible to change credential");
            exit(255);
        }
        if(args.n && args.args[args.n - 1][0] == '@') {
            if(setpriority(PRIO_PROCESS, getpid(), atoi(&args.args[args.n - 1][1])) == -1) {
                perror("Unable to set priority"); 
                exit(255);
            }
            args.n--;
            args.args[args.n] = NULL;
        }
        char** nargs = malloc((args.n + 2) * sizeof(char*));
        nargs[0] = pog;
        for(int i = 1; i <= args.n; i++) nargs[i] = args.args[i - 1];
        nargs[args.n + 1] = NULL;
        if(execvp(pog, nargs) == -1) printf("Unable to execute %s: %s\n", pog, strerror(errno));
        exit(255);
    } else {
        if(args.n && args.args[args.n - 1][0] == '@') {
            args.n--;
            args.args[args.n] = NULL;
        }
        node.next = NULL;
        node.pid = knight;
        node.time = time(NULL);
        node.state = 0;
        node.has_exited = false;

        siz = temp = strlen(pog) + 1;
        temp2 = malloc(args.n * sizeof(size_t));
        for(int i = 0; i < args.n; i++) siz += temp2[i] = strlen(args.args[i]) + 1;
        node.cmd = malloc(siz);
        strncpy(node.cmd, pog, temp);
        spc = node.cmd + temp;
        *(spc - 1) = args.n ? ' ' : '\0';
        for(int i = 0; i < args.n; i++) {
            strncpy(spc, args.args[i], temp2[i]);
            spc += temp2[i];
            *(spc - 1) = ' ';
        }
        *(spc - 1) = '\0';
        free(temp2);
        
        proc_list_append(&proc_data, node);
    }
}

void c_background(cmd_args args) {
    if(!args.n) {
        printf("Parameter prog is required\n");
        return;
    }
    char* pog = args.args[0];
    args.n--;
    args.args = &args.args[1];
    new_bg_process(pog, args, -1);
}

void c_runas(cmd_args args) {
    uid_t uid; pid_t knight; char* temp;
    if(args.n < 2) {
        printf("Parameters prog and login are required\n");
        return;
    }
    if ((uid = useruid(args.args[0])) == (uid_t)-1) {
        printf("User %s not found\n", args.args[0]);
        return;
    }
    if (seteuid(uid) == -1) {
        perror("Impossible to change credential");
        return;
    }

    if(args.n > 2 && !strncmp("&", args.args[args.n - 1], 2)) {
        temp = args.args[1];
        args.args[args.n - 1] = NULL;
        args.args = &args.args[2];
        args.n -= 3;
        new_bg_process(temp, args, uid);
        return;
    }

    if((knight = fork()) == -1) {
        perror("Unable to fork");
        return;
    } else if (!knight) {
        //Child process
        if (seteuid(uid) == -1) {
            perror("Impossible to change credential");
            exit(255);
        }
        if(args.n > 2 && args.args[args.n - 1][0] == '@') {
            if(setpriority(PRIO_PROCESS, getpid(), atoi(&args.args[args.n - 1][1])) == -1) {
                perror("Unable to set priority"); 
                exit(255);
            }
            args.n--;
            args.args[args.n] = NULL;
        }
        if(execvp(args.args[1], &args.args[1]) == -1) printf("Unable to execute %s: %s\n", args.args[1], strerror(errno));
        exit(255);
    } else {
        //Parent process
        if(waitpid(knight, NULL, 0) == -1) perror("Unable to wait");
    }
}

void c_executeas(cmd_args args) {
    uid_t uid;
    if(args.n < 2) {
        printf("Parameters prog and login are required\n");
        return;
    }
    if ((uid = useruid(args.args[0])) == (uid_t)-1) {
        printf("User %s not found\n", args.args[0]);
        return;
    }
    if (seteuid(uid) == -1) {
        perror("Impossible to change credential");
        return;
    }
    if(args.n > 2 && args.args[args.n - 1][0] == '@') {
        if(setpriority(PRIO_PROCESS, getpid(), atoi(&args.args[args.n - 1][1])) == -1) {
            perror("Unable to set priority"); 
            return;
        }
        args.n--;
        args.args[args.n] = NULL;
    }
    if(execvp(args.args[1], args.args) == -1) printf("Unable to execute %s: %s\n", args.args[1], strerror(errno));
}

void c_listprocs() {
    proc_list_print(proc_data);
}

void c_proc(cmd_args args) {
    pid_t pid; int state; proc_pos node;
    if(!args.n) {
        proc_list_print(proc_data);
        return;
    }
    if(!strncmp("-fg", args.args[0], 4)) {
        if(args.n < 2 || !(pid = atoi(args.args[1])) || waitpid(pid, &state, 0) == -1) {
            if(proc_list_get(proc_data, pid)) printf("Process couldn't be brought to the foreground\n");
            proc_list_print(proc_data);
            return;
        }
        node = proc_list_get(proc_data, pid);
        if(!node) {
            printf("Process node couldn't be found\n");
            return;
        }
        node->has_exited = true;
        node->state = state;
        proc_node_print(node);
        proc_list_remove_node(&proc_data, pid);
    } else {
        if(!(pid = atoi(args.args[0])) || !(node = proc_list_get(proc_data, pid))) {
            proc_list_print(proc_data);
            return;
        }
        proc_node_print(node);
    }
}

void c_deleteprocs(cmd_args args) {
    if(!args.n) {
        printf("Specifying the type to remove is required\n");
        return;
    }
    if(!strncmp(args.args[0], "-term", 6)) proc_list_remove_exi(&proc_data);
    else if(!strncmp(args.args[0], "-sig", 5)) proc_list_remove_sig(&proc_data);
    else printf("Invalid remove type %s\n", args.args[0]);
}

void c_generic(char* cmd, cmd_args args) {
    pid_t knight;
    if(args.n && !strncmp("&", args.args[args.n - 1], 2)) {
        args.n--;
        args.args[args.n] = NULL;
        new_bg_process(cmd, args, -1);
        return;
    }

    if((knight = fork()) == -1) {
        perror("Unable to fork");
        return;
    } else if (!knight) {
        //Child process
        if(args.n && args.args[args.n - 1][0] == '@') {
            if(setpriority(PRIO_PROCESS, getpid(), atoi(&args.args[args.n - 1][1])) == -1) {
                perror("Unable to set priority"); 
                exit(255);
            }
            args.n--;
            args.args[args.n] = NULL;
        }

        char** nargs = malloc((args.n + 2) * sizeof(char*));
        nargs[0] = cmd;
        for(int i = 1; i <= args.n; i++) nargs[i] = args.args[i - 1];
        nargs[args.n + 1] = NULL;

        if(execvp(cmd, nargs) == -1) printf("Unable to execute %s: %s\n", cmd, strerror(errno));
        exit(255);
    } else {
        //Parent process
        if(waitpid(knight, NULL, 0) == -1) perror("Unable to wait");
    }
}

bool process_cmd(cmd_list *history, char* in, uint8_t from_historic) {
    if(from_historic && !strncmp(in, "historic", 8)) {
        printf("Historic calls don't support recursion\n");
        return false;
    }
    char *params; cmd_args args;
    if(in[0] != ' ' && in[0] != '\n' && in[0] != '\0') cmd_list_insert(history, in);
    uint64_t h = hash((unsigned char *) in, CMD_LENGTH, &params);
    param_to_args(params, &args);
    switch(h) {
        case H_GETPID:
            c_getpid();
            break;
        case H_GETPPID:
            c_getppid();
            break;
        case H_PWD:
            c_pwd();
            break;
        case H_CHDIR:
            c_chdir(args);
            break;
        case H_DATE:
            c_date();
            break;
        case H_TIME:
            c_time();
            break;
        case H_EXIT: case H_QUIT: case H_END:
            if(args.args) free(args.args);
            return true;
        case H_AUTHORS:
            c_authors(args);
            break;
        case H_HISTORIC:
            c_historic(args, history, in);
            break;
        case H_CREATE:
            c_create(args);
            break;
        case H_DELETE:
            c_delete(args);
            break;
        case H_LIST:
            c_list(args);
            break;
        case H_MEMORY:
            c_memory(args);
            break;
        case H_RECURSE:
            c_recurse(args);
            break;
        case H_MEMDUMP:
            c_memdump(args);
            break;
        case H_MEMFILL:
            c_memfill(args);
            break;
        case H_READFILE:
            c_readfile(args);
            break;
        case H_WRITEFILE:
            c_writefile(args);
            break;
        case H_GETPRIORITY:
            c_getpriority(args);
            break;
        case H_SETPRIORITY:
            c_setpriority(args);
            break;
        case H_GETUID:
            c_getuid();
            break;
        case H_SETUID:
            c_setuid(args);
            break;
        case H_FORK:
            c_fork();
            break;
        case H_EXECUTE:
            c_execute(args);
            break;
        case H_FOREGROUND:
            c_foreground(args);
            break;
        case H_BACKGROUND:
            c_background(args);
            break;
        case H_RUNAS:
            c_runas(args);
            break;
        case H_EXECUTEAS:
            c_executeas(args);
            break;
        case H_LISTPROCS:
            c_listprocs();
            break;
        case H_PROC:
            c_proc(args);
            break;
        case H_DELETEPROCS:
            c_deleteprocs(args);
            break;
        default:
            //printf("Command not found. F in the shell.\n"); F
            if(in[0] == '\0') break;
            c_generic(in, args);
            break;
    }
    if(args.args) free(args.args);
    return false;
}

void set_int_vector(void * h) {
    struct sigaction act;
    act.sa_handler = h;
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) == -1) perror("Unable to set interrupt vector");
}

int main() {
    set_int_vector(&sig_handler);
    
    char in[CMD_LENGTH]; cmd_list history;
    free_on_interrupt = &history;
    cmd_list_init(&history);
    mem_list_init(&mem_data);
    proc_list_init(&proc_data);

    do {
        printf("\033[1;34m→\033[0m  ");
    } while (!read_input(in, CMD_LENGTH) && !process_cmd(&history, in, 0));
    
    cmd_list_reset(&history); //Frees up heap allocated list.
    mem_list_reset(&mem_data);
    proc_list_reset(&proc_data);
    printf("\n");
    return 0;
}