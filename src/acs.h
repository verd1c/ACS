#ifndef __ACS_H__
#define __ACS_H__

#define SYS_CALL_TABLE_F    "/usr/include/x86_64-linux-gnu/asm/unistd_32.h"

#define TABLE_SIZE          64
#define HASH_MULTIPLIER     65599

#define BUFFLEN             256

typedef struct SysCall SysCall;
typedef struct Instructions Instructions;
typedef struct ACS ACS;

int acs_init(ACS* a, char* filename);
int acs_monitor();

struct SysCall {
    const char *name;       // syscall name
    int code;               // syscall number
    int allowance;          // times allowed to be run per sec

    struct SysCall* next;
    struct SysCall* nextInCombo;
};

struct Instructions{
    SysCall* table[TABLE_SIZE];
    long length;
};

struct ACS {
    Instructions* instr;

    SysCall* combo; // combo list
    int cc; // combo counter
};

#endif