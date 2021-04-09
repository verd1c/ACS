#include <stdio.h>
#include <stdlib.h>
#include "acs.h"

static Instructions *instr;

int acs_init_structions(char *filename){
    FILE *fp;
    char syscall[50];
    int line_no = 0;

    /* init instructions */
    instr = (Instructions*)malloc(sizeof(Instructions));
    if(!instr){
        fprintf(stderr, "acs: error: memory allocation failed");
        return 0;
    }

    fp = fopen(filename, "r");




    fclose(fp);

    return 1;
}

int acs_monitor(){

}