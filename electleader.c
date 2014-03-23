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
    bool debug = false;
    bool leader_elected, participant, is_leader;
    int PNUM, id, send_count, recv_count, world_size, world_rank, leader_id, message;

    // Parse required arguments
    if (debug) for (int i=0; i<=argc-1; i++) printf("arg%i:%s\n", i, argv[i]);
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

    // START PROGRAM CODE
    leader_elected = false;
    participant = false;
    is_leader = false;
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    // Find out rank, size
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    id = (world_rank+1) * PNUM % world_size;
    leader_id = -1;
    message = -1;

    while (leader_elected == false) {
        if (world_rank != 0) {
            int last_message = message;
            MPI_Recv(&message, 5, MPI_INT, world_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Process %d received message %d from process %d\n", world_rank, message, world_rank - 1);
            if (message > id) {
                participant = true;
            } else if ((message < id) && participant == false) {
                message = id;
                participant = true;
            } else if ((message < id) && participant == true) {
                // TODO: discard message
                message = last_message;
            } else if (message == id) {

            }
          } else {
            // Set the message's value if you are process 0
            if (message == -1) message = id;
          }
          MPI_Send(&message, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);
          // Now process 0 can receive from the last process. This makes sure that at
          // least one MPI_Send is initialized before all MPI_Recvs (again, to prevent
          // deadlock)
          if (world_rank == 0) {
            MPI_Recv(&message, 5, MPI_INT, world_size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Process %d received message %d from process %d\n", world_rank, message, world_size - 1);
            if (message == id) {

            }
          }
    }
    MPI_Finalize();
        printf("%d: %d\n", world_rank, leader_id);

}



