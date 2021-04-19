#include <stdio.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


#define gettid() syscall(SYS_gettid)

int main(){
    pid_t pid;

    pid = getpid();

    printf("%d\n", pid);

    return 1;
}