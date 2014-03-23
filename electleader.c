//
//  electleader.c 
//  cpsc-assignment3
//
//  Created by Evan Louie (m6d7) on 2014-03-21.
//  Copyright (c) 2014 Evan Louie. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <mpi.h>
#define MASTER  0
int print_error(char *message) {
    printf("===========ERROR==========\n\t%s\n==========================\n", message);
    exit(-1);
}
int debug_message(char *message) {
    printf("%s\n", message);
    return 0;
}
int main(int argc, char * argv[])
{
    bool debug = true;
    
    int PNUM = -1;
    // Parse required arguments
    for (int i=0; i<=argc-1; i++) {
        printf("arg%i:%s\n", i, argv[i]);
    }
    
    if (argv[1]==NULL) {
        print_error("PNUM (arg1) not provided");
    } else {
        bool is_numeric = true;
        for (int i=0; i<strlen(argv[1]); i++) {
            if (isdigit(argv[1][i])) {
            } else {
                is_numeric = false;
                print_error("Non digit found in arg1");
            }
        }
        if (is_numeric) {
            PNUM = atoi(argv[1]);
        }
    }
    
    if (debug) {
        printf("debug-mode: active\n");
    }
    
    // insert code here...
    printf("Hello, World!\n");

   int rank, numprocs;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   printf("Hello World from proces %d of %d.\n",rank,numprocs);
   MPI_Finalize();

    return 0;
}



