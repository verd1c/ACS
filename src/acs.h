#ifndef __ACS_H__
#define __ACS_H__

#define TABLE_SIZE 64

int acs_init_structions(char *filename);

typedef struct SysCall_t {
    const char *name;       // syscall name
    long code;              // syscall number
    int allowance;          // times allowed to be run per sec
} SysCall;

typedef struct Instructions_t{
    SysCall table[TABLE_SIZE];
    long length;
} Instructions;

SysCall insert(Instructions *i, char *name, long no, int allowance);
SysCall lookup_name(Instructions *i, char *name);
SysCall lookup_code(Instructions *i, long code);

#endif