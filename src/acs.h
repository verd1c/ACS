#ifndef __ACS_H__
#define __ACS_H__


#define SYS_CALL_TABLE_F    "/usr/include/x86_64-linux-gnu/asm/unistd_64.h"

#define TABLE_SIZE          64
#define HASH_MULTIPLIER     65599

#define BUFFLEN             256

#define max(a,b) a > b ? a : b

typedef struct SysCall SysCall;
typedef struct Instructions Instructions;
typedef struct HistoryEntry HistoryEntry;
typedef struct SysCallQueue SysCallQueue;
typedef struct ACS ACS;

int acs_init(ACS *a, char *filename);
int acs_monitor(ACS *a, char *filename);

struct SysCall {
    const char *name;       // syscall name
    int code;               // syscall number
    int allowance;          // times allowed to be run per sec
    long timestamp;         // seconds till last hour

    struct SysCall* next;
    struct SysCall* nextInCombo;
};

struct Instructions{
    SysCall* table[TABLE_SIZE];
    long length;
};

struct HistoryEntry {
    long timestamp;
    long code;
};

struct SysCallQueue {
    long front, rear, size, capacity;
    HistoryEntry **array;
};

struct ACS {
    unsigned char is_triggered;

    Instructions *instr;

    SysCallQueue *history;

    SysCall *combo; // combo list
    SysCall *currCombo; // combo counter
};

#endif