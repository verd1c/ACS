#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include "acs.h"

static inline int hash(long code){
    return (code * HASH_MULTIPLIER) % TABLE_SIZE;
}

void print_instr(Instructions* instr){
    SysCall* s;
    int i;

    printf("=================================\n");
    printf("Sys Call Table\n");
    printf("=================================\n");
    for(i = 0; i < TABLE_SIZE; i++){
        if(instr->table[i] == NULL){
            continue;
        }

        s = instr->table[i];

        while(s != NULL){
            printf("SysCall [%-30s] [%-7d] [%-7d] \n", s->name, s->code, s->allowance);

            s = s->next;
        }
    }
}

void print_combo(ACS* a){
    SysCall* s;

    printf("=================================\n");
    printf("Combo List\n");
    printf("=================================\n");
    s = a->combo;
    while(s){
        printf("SysCall [%-30s] [%-7d] [%-7d] \n", s->name, s->code, s->allowance);
        s = s->nextInCombo;
    }
}

SysCall* lookup_code(Instructions* instr, long code){
    SysCall* s;
    int h;

    h = hash(code);

    s = instr->table[h];

    while(s){
        if(s->code == code)
            return s;

        s = s->next;
    }

    return NULL;
}

SysCall* lookup_name(Instructions* instr, char* name){
    SysCall* s;
    int i;

    // iterate everything
    for(i = 0; i < TABLE_SIZE; i++){
        s = instr->table[i];

        while(s){
            if(strcmp(s->name, name) == 0)
                return s;

            s = s->next;
        }
    }

    return NULL;
}

SysCall* insert(Instructions* instr, char* name, long code, int allowance){
    SysCall* iter;
    int h;

    h = hash(code);

    if(instr->table[h] == NULL){
        instr->table[h] = (SysCall*)malloc(sizeof(SysCall));
        instr->table[h]->name = strdup(name);
        instr->table[h]->code = code;
        instr->table[h]->allowance = allowance;
        instr->table[h]->next = NULL;

        iter = instr->table[h];
    }else{
        iter = instr->table[h];

        while(iter->next != NULL){
            iter = iter->next;
        }

        iter->next = (SysCall*)malloc(sizeof(SysCall));
        iter->next->name = strdup(name);
        iter->next->code = code;
        iter->next->allowance = allowance;
        iter->next->next = NULL;

        iter = iter->next;
    }

    return iter;
}

static void trim_name(char* dst, char* src){
    int len, i, j;
    len = strlen(src);

    if(len < 6){
        return;
    }

    for(i = 5, j = 0; i < len; i++){
        dst[j++] = src[i];
    }

    return;
}

// initialize syscalls using defined file
int init_syscalls(Instructions* instr){
    FILE* fp;
    char c, *token, line[BUFFLEN];
    int count;

    // syscall specific
    char name[BUFFLEN];
    int code = -1;

    fp = fopen(SYS_CALL_TABLE_F, "r");

    while(fgets(line, sizeof(line), fp)){
        
        // tokenize
        token = strtok(line, " ");
        count = 0;
        memset(name, '\0', BUFFLEN);
        while(token != NULL){
            if(count == 1){
                trim_name(name, token);
            }else if(count == 2){
                code = atoi(token);
            }

            token = strtok(NULL, " ");
            count++;
        }

        if(count == 3 && strstr(name, "UNISTD_32") == NULL){
            insert(instr, name, code, -1);
        }

    }

    fclose(fp);

    return 1;
}

Instructions* init_instructions(void){
    Instructions* iTable;
    int i;

    iTable = (Instructions*)malloc(sizeof(Instructions));
    for(i = 0; i < TABLE_SIZE; i++){
        iTable->table[i] = NULL;
    }

    iTable->length = 0;

    return iTable;
}

void add_combo(ACS* a, SysCall* s){
    SysCall* iter;

    iter = a->combo;
    s->nextInCombo = (SysCall*)0;

    if(!iter){
        a->combo = s;
        a->currCombo = s;
        return;
    }

    while(iter->nextInCombo){
        iter = iter->nextInCombo;
    }

    iter->nextInCombo = s;
    return;
}

int setup_instructions(ACS* a, char *filename){
    FILE *fp;
    char c, chain[BUFFLEN * 2], *token, syscall[BUFFLEN];
    int line_no = 0, count = 0, allowance;
    SysCall* s;

    fp = fopen(filename, "r");

    /* read first line */
    memset(chain, '\0', BUFFLEN * 2);
    c = getc(fp);
    while(c != '\n' && c != EOF){
        chain[count++] = c;
        c = getc(fp);
    }

    // tokenize first line

    token = strtok(chain, " ");
    while(token != NULL){
        // create combo syscall
        s = (SysCall*)malloc(sizeof(SysCall));
        s->name = strdup(token);
        s->code = lookup_name(a->instr, token)->code;
        s->allowance = -1;
        s->next = (SysCall*)0;
        s->nextInCombo = (SysCall*)0;

        add_combo(a, s);
        token = strtok(NULL, " ");
    }

    while(fscanf(fp, "%s %d", syscall, &allowance) == 2){
        s = lookup_name(a->instr, syscall);
        s->allowance = allowance;
    }

    fclose(fp);

    return 1;
}

int trigger(){

}

int acs_init(ACS* a, char* filename){
    a->instr = (Instructions*)0;
    a->combo = (SysCall*)0;
    a->currCombo = (SysCall*)0;

    if(!a)
        return 0;

    a->instr = init_instructions();

    init_syscalls(a->instr);

    setup_instructions(a, filename);

    //print_instr(a->instr);
    //print_combo(a);

    return 1;
}

int acs_monitor(ACS* a, char* filename){
    pid_t child;
    int syscall;
    struct user_regs_struct regs;

    child = fork();
    if(child == 0){
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execv(filename, NULL);
    }else{
        while(1){
            wait(NULL); // wait child to get sig
            syscall = ptrace(PTRACE_PEEKUSER, child, sizeof(long) * RAX);
            printf("%ld\n", syscall);
        }
    }
    return 0;
}