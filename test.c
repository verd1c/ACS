#include <stdio.h>
#include <stdlib.h>
#include "src/acs.h"

int main(int argc, char **argv){
    ACS a;

    if(argc < 3){
        printf("acs: error: wrong usage!\n");
        exit(0);
    }

    if(!acs_init(&a, argv[1])){
        printf("acs: error: initialization failed, exiting!\n");
        exit(0);
    }

    if(!acs_monitor(&a, argv[2])){
        printf("acs: error: monitoring failed, exiting!\n");
        exit(0);
    }

    return 0;
}