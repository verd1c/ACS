#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int setup_instructions(Instructions* instr, char *filename){
    FILE *fp;
    char c, chain[200];
    char syscall[50];
    int line_no = 0, count = 0, allowance;
    SysCall* s;

    fp = fopen(filename, "r");

    /* read first line */
    c = getc(fp);
    while(c != '\n' && c != EOF){
        chain[count++] = c;
        printf("%c", c);
        c = getc(fp);
    }
    printf("\n");

    while(fscanf(fp, "%s %d", syscall, &allowance) == 2){
        s = lookup_name(instr, syscall);
        s->allowance = allowance;
    }

    fclose(fp);

    return 1;
}

int trigger(){

}

int acs_init(ACS* a, char* filename){

    if(!a)
        return 0;

    a->instr = init_instructions();

    init_syscalls(a->instr);

    setup_instructions(a->instr, filename);

    return 1;
}

int acs_monitor(){

}