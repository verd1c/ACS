#include <stdio.h>
#include <stdlib.h>
#include "src/acs.h"

int main(int argc, char **argv){

    if(argc < 2){
        printf("acs: error: wrong usage!\n");
        exit(0);
    }

    if(!acs_init(argv[1])){
        printf("acs: error: initialization failed, exiting!\n");
        exit(0);
    }

    return 0;
}