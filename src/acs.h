#ifndef __ACS_H__
#define __ACS_H__

#define SYS_CALL_TABLE_F    "/usr/include/x86_64-linux-gnu/asm/unistd_32.h"

#define TABLE_SIZE          64
#define HASH_MULTIPLIER     65599

#define BUFFLEN             256

int acs_init(char *filename);
int acs_monitor();

typedef struct SysCall_t {
    const char *name;       // syscall name
    int code;               // syscall number
    int allowance;          // times allowed to be run per sec

    struct SysCall_t* next;
    struct SysCall_t* nextInCombo;
} SysCall;

typedef struct Instructions_t{
    SysCall* table[TABLE_SIZE];
    long length;
} Instructions;

#endif