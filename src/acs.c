#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <time.h>
#include <assert.h>
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

// SysCallQueue

SysCallQueue* create_history(long capacity){
    SysCallQueue *history = (SysCallQueue*)malloc(sizeof(SysCallQueue));
    history->capacity = capacity;
    history->front = 0;
    history->size = 0;

    history->rear = capacity - 1;
    history->array = (HistoryEntry**)malloc(history->capacity * sizeof(HistoryEntry*));
    return history;
}

int isFull(SysCallQueue *h){
    return (h->size == h->capacity);
}

int isEmpty(SysCallQueue *h){
    return (h->size == 0);
}

HistoryEntry* dequeue(SysCallQueue *h){
    if(isEmpty(h)){
        return NULL;
    }
    HistoryEntry *i = h->array[h->front];
    h->front = (h->front + 1)%h->capacity;
    h->size = h->size - 1;
    return i;
}

void enqueue(SysCallQueue *h, HistoryEntry *s){
    if(isFull(h)){
        dequeue(h);
    }
    
    h->rear = (h->rear + 1)%h->capacity;
    h->array[h->rear] = s;
    h->size = h->size + 1;
    //printf("Enqueued %s\n", s->name);
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
    char *token, line[BUFFLEN];
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

        if(count == 3 && strstr(name, "X86_UNISTD") == NULL){
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
    int count = 0, allowance, maxCombo = 0, maxAllowance = -1;
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
        maxCombo++;

        add_combo(a, s);
        token = strtok(NULL, " ");
    }

    while(fscanf(fp, "%s %d", syscall, &allowance) == 2){
        s = lookup_name(a->instr, syscall);
        s->allowance = allowance;
        if(allowance > maxAllowance){
            maxAllowance = allowance;
        }
    }

    a->history = create_history(max(maxCombo, maxAllowance));

    fclose(fp);

    return 1;
}

void print_history(ACS *a){
    SysCallQueue *h = a->history;
    HistoryEntry *s;
    SysCall *c;
    int i, iter;

    printf("========================\n");
    printf("Queue\n");
    printf("========================\n");
    iter = 0;
    i = h->front;
    while(iter < h->size){
        s = h->array[i];

        c = lookup_code(a->instr, s->code);

        if(!c){
            continue;
        }

        printf("HistoryEntry [%-15s] : [%-7ld] -> [%-7ld]\n", c->name, s->code, s->timestamp);
        i = (i + 1) % h->capacity;
        iter++;
    }
}

// 1 if combo triggered, 0 if not
int check_combo(ACS *a){
    SysCall *combo = a->combo;
    HistoryEntry *s, *d;
    SysCallQueue *h = a->history;
    int i, iter, j, jiter, count;

    iter = 0;
    i = h->front;
    while(iter < h->size){

        s = h->array[i];

        combo = a->combo;
        count = 0;
        if(combo->code == s->code){
            
            jiter = iter + 1;
            j = (i + 1) % h->capacity;
            combo = combo->nextInCombo;
            while(jiter < h->size){
                d = h->array[j];

                if(combo->code == d->code && combo->nextInCombo == NULL)
                    return 1;

                if(combo->code == d->code){
                    combo = combo->nextInCombo;
                    count++;
                }
                j = (j + 1) % h->capacity;
                jiter++;
            }
        }

        i = (i + 1) % h->capacity;
        iter++;
    }
    return 0;
}

void trigger(ACS *a){
    a->is_triggered = 1;
    printf("Acess Control System triggered, monitoring carefully\n");
    return;
}

int check_breach(ACS *a){
    SysCallQueue *h = a->history;
    HistoryEntry *e, *d;
    SysCall *s;
    int i, iter, j, jiter, counter;

    iter = 0;
    i = h->front;
    while(iter < h->size){
        // [][][][][i, 21][j, 21][i, 21][i, 22]
        e = h->array[i];
        counter = 1;
        s = lookup_code(a->instr, e->code);

        // iterate eveyrthign else
        jiter = iter + 1;
        j = (i + 1) % h->capacity;
        while(jiter < h->size){
            d = h->array[j];

            if(e->code == d->code && e->timestamp == d->timestamp){
                counter++;
            }
            
            j = (j + 1) % h->capacity;
            jiter++;
        }


        if(s->allowance != -1 && counter > s->allowance){
            return 1;
        }

        i = (i + 1) % h->capacity;
        iter++;
    }

    return 0;
}

int trace(ACS *a, pid_t child){
    int syscall, status, counter = 0;
    time_t rawtime;
    struct tm *timeinfo;
    SysCall *s, *d;
    HistoryEntry *e;

    waitpid(child, &status, 0); // skip execve
    ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
    while(1){

        // wait child to get sig
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);

        // break on duplicate
        if(counter++ % 2 != 0)
            continue;

        // break if exit
        if(WIFEXITED(status))
            break;


        // orig_eax on 32bit, orig_rax on 64bit
        syscall = ptrace(PTRACE_PEEKUSER, child, 4 * ORIG_EAX);

        s = lookup_code(a->instr, syscall);
        if(!s){
            continue;
        }

        // got syscall
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        e = (HistoryEntry*)malloc(sizeof(HistoryEntry));
        e->code = s->code;
        e->timestamp = timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;

        enqueue(a->history, e);
        printf("----------------------------\nSys: %ld : %s at timestamp: %ld\n", syscall, s->name, timeinfo->tm_sec);
        //print_history(a);
        //print_history(a);
        if(!a->is_triggered){
            // check combo
            if(check_combo(a))
                // trigger
                trigger(a);
        }else{
            //already triggered
            if(check_breach(a)){
                printf("Breached at:\n");
                print_history(a);
            }
        }
    }

}

int acs_init(ACS* a, char* filename){
    a->instr = (Instructions*)0;
    a->combo = (SysCall*)0;
    a->currCombo = (SysCall*)0;
    a->is_triggered = 0;

    if(!a)
        return 0;

    a->instr = init_instructions();

    init_syscalls(a->instr);

    setup_instructions(a, filename);
    a->currCombo = a->combo;

    //print_instr(a->instr);
    //print_combo(a);

    return 1;
}

int acs_monitor(ACS* a, char* filename){
    pid_t child;
    int syscall;
    SysCall *s;

    child = fork();
    if(child == 0){
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execv(filename, NULL);
    }else{
        return trace(a, child);
    }
    return 0; 
}