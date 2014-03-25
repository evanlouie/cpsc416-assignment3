//
//  electleader.c 
//  cpsc-assignment3
//
//  Created by Evan Louie (m6d7) on 2014-03-21.
//  Copyright (c) 2014 Evan Louie. All rights reserved.
//
/*
    Notes to reader:
    ================
    You will also find electleader_uni.c which contains a 100% working implementatino of vanilla uni directional chang-roberts.
    Rename it to electleader.c to use that instead.

    In this file: 
    Attempted bi-directional implementation of a modified chang-roberts algorithm.  The algorith would work by a 
    process (in this case rank 0) sending the initial election message to both its neighbors.  The neighbors
    would subsequently pass the message along based upon the direction it came from (0->12->11 || 0->1->2). 
    When a process recieved messages from both ends, it would choose the largest message and designate it as leader.
    Said process would then send elected messages in both directions and said messages would be passed along like
    the election-initiaion message.  Once a process received the elected message on both sides, the algorithm terminates.

    Performance:
    ------------
    This implementation is generally faster then the uni-directional as it should be able to find the leader by half way across the ring.
    HOWEVER: this application is glitchy and doesn't work perfectly.

    General Issues:
    --------------
    - This application is generally buggy.
    - It sometimes reaches a livelock at some point during the final iteration to sum the total recieves and sends throughout the entire
    application. Reaching this lock seems to be dependant upon the combination of PNUM and NUM used.
    - For unknown reason, termination doesn't always occure when a process recieves a message from both sides.  As such, some extra 
    messages are sent.


    KNONW ISSUES:
    -------------
    i) Sometimes, a process hangs and doesn't finalize();
    ii) During one of the iterations, process 0 seems to overwrite its leader_id to something incorrect.  Unable to find reason. 
    iii) total recv count is off
    
    NOTE: NONE OF THESE ISSUES ARE FOUND IN VANILLA UNIDIRECTIONAL CHANG-ROBERTS IMPLEMNTATION FOUND IN electleader_uni.c

*/

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


bool debug;
int print_error(char *message) {
    printf("===========ERROR==========\n\t%s\n==========================\n", message);
    exit(-1);
}
int debug_message(char *message) {
    printf("%s\n", message);
    return 0;
}

int send_election_message(int *message, bool *participant, int * world_rank, int * world_size, int * send_count) {
    MPI_Send(message, 1, MPI_INT, (*world_rank + 1) % *world_size, 0, MPI_COMM_WORLD);
    *participant = true;
    *send_count += 1;
    return *message;
}

int send_elected_message(int *message, int * world_rank, int * world_size, int *send_count) {
    MPI_Send(message, 1, MPI_INT, (*world_rank + 1) % *world_size, 0, MPI_COMM_WORLD);
    *send_count += 1;

    return *message;
}



int send_message(int *message, int *world_rank, int *world_size, int *target, int *send_count) {
    MPI_Send(message, 1, MPI_INT, *target, 0, MPI_COMM_WORLD);
    *send_count += 1;
    // if (debug) printf("%d send %d to %d\n", *world_rank, *message, *target);
    return 1;
}

int pass_message(int *message, int *world_rank, int *world_size, MPI_Status *status, int *send_count) {
    int target;
    if (status->MPI_SOURCE < *world_rank) {
        if (*world_rank == *world_size-1) {
            target = (*world_rank -1) % *world_size;
        } else {
            target = (*world_rank +1) % *world_size;
        }
    } else if (status->MPI_SOURCE > *world_rank) {
        target = (*world_rank - 1) % *world_size;
    } else {
        print_error("INVALID: MESSAGE SENT TO SELF\n");
    }
    if (target < 0) target *= -1;

    send_message(message, world_rank, world_size, &target, send_count);
    return 1;
}

int send_message_both_ways(int *message, int *world_rank, int *world_size, MPI_Status *status, int *send_count) {
    int target = (*world_rank + 1) % *world_size;
    send_message(message, world_rank, world_size, &target, send_count);
    target = (*world_rank + *world_size - 1) % *world_size;
    send_message(message, world_rank, world_size, &target, send_count);

    return 1;
}

int recv_message(int *message, int *last_message, int *recv_count, MPI_Status *status, int *world_rank, int *world_size, bool *recv_left, bool *recv_right) {
    *last_message = *message;
    MPI_Recv(message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    *recv_count += 1;
    if (status->MPI_SOURCE < *world_rank) {
        if (*world_rank == *world_size -1) {
            *recv_right = true;
        } else {
            *recv_left = true;
        }
    } else {
        *recv_right = true;
    }

    if (debug) printf("%d recv %d from %d\n", *world_rank, *message, status->MPI_SOURCE);
    return 1;
}

int main(int argc, char * argv[])
{
    debug = false;
    bool participant, is_leader;
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
    participant = false;
    is_leader = false;
    send_count = 0;
    recv_count = 0;
    bool election_started = false;
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    id = (world_rank+1) * PNUM % world_size;
    leader_id = 0;
    message = 0;
    int last_message = message;

    MPI_Status status;
    bool recv_left = false;
    bool recv_right = false;
    while (leader_id == 0) {
        if (world_rank != 0) {
            recv_message(&message, &last_message, &recv_count, &status, &world_rank, &world_size, &recv_left, &recv_right);
            // printf("Process %d received message %d from process %d\n", world_rank, message, world_rank - 1);
            
            if (message < 0) {
                // CATCH FOR ELECTED MESSAGE
                participant = false;
                leader_id = message*-1;
                if (leader_id != id) pass_message(&message, &world_rank, &world_size, &status, &send_count);
                printf("rank=%d, id=%d, leader=%d, mrcvd=%d, msent=%d\n", world_rank, id, leader_id, recv_count, send_count);
            } else if (message > id) {
                pass_message(&message, &world_rank, &world_size, &status, &send_count);
            } else if ((message < id) && participant == false) {
                message = id;
                pass_message(&message, &world_rank, &world_size, &status, &send_count);
            } else if ((message < id) && participant == true) {
                message = last_message;
                pass_message(&message, &world_rank, &world_size, &status, &send_count);
            } else if (recv_left && recv_right) {
                if (debug) printf("recv from both sides\n");
                if (message < last_message) message = last_message;
                leader_id = message;
                // START SECOND PART OF ALGORITHM (ELECTED MESSAGE)
                participant = false;
                if (message > 0) {
                    message = message * -1;

                }
                send_message_both_ways(&message, &world_rank, &world_size, &status, &send_count);
                printf("rank=%d, id=%d, leader=%d, mrcvd=%d, msent=%d\n", world_rank, id, leader_id, recv_count, send_count);
            }
        } else {
            // Set the message's value if you are process 0
            if (election_started == false) {
                if (message == 0) message = id;
                send_message_both_ways(&message, &world_rank, &world_size, &status, &send_count);
                election_started = true;
            }
            
            recv_message(&message, &last_message, &recv_count, &status, &world_rank, &world_size, &recv_left, &recv_right);
            // printf("Process %d received message %d from process %d\n", world_rank, message, world_rank - 1);
            
            if (message < 0) {
                // CATCH FOR ELECTED MESSAGE
                participant = false;
                leader_id = message*-1;
                // send_elected_message(&message, &world_rank, &world_size, &send_count);
                if (message > 0) message *= -1;
                if (message != id) pass_message(&message, &world_rank, &world_size, &status, &send_count);
                printf("rank=%d, id=%d, leader=%d, mrcvd=%d, msent=%d\n", world_rank, id, leader_id, recv_count, send_count);
            } else if (message > id) {
                pass_message(&message, &world_rank, &world_size, &status, &send_count);
            } else if ((message < id) && participant == false) {
                message = id;
                pass_message(&message, &world_rank, &world_size, &status, &send_count);
            } else if ((message < id) && participant == true) {
                // TODO: discard message
                message = last_message;
                pass_message(&message, &world_rank, &world_size, &status, &send_count);
            } else if (recv_left && recv_right) {
                if (message < last_message) message = last_message;
                leader_id = message;
                // START SECOND PART OF ALGORITHM (ELECTED MESSAGE)
                participant = false;
                message = message*-1;
                send_message_both_ways(&message, &world_rank, &world_size, &status, &send_count);
                printf("rank=%d, id=%d, leader=%d, mrcvd=%d, msent=%d\n", world_rank, id, leader_id, recv_count, send_count);
            }
        }
    }
    int total_send_count = send_count;
    int total_recv_count = recv_count*-1;
    if (leader_id == id) {
        MPI_Send(&total_send_count, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);
        MPI_Send(&total_recv_count, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);
        
        for (int i = 0; i<=1; i++) {
            MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (message > 0) {
                total_send_count = message;
            } else {
                total_recv_count = message*-1;
            }
        }

        printf("rank=%d, id=%d, trcvd=%d, tsent=%d\n", world_rank, id, total_recv_count, total_send_count);
    } else {

        for (int i=0; i<=1; i++) {
            MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (message > 0) {
                total_send_count += message;
            } else {
                total_recv_count += message;
            }
        }
        MPI_Send(&total_send_count, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);
        MPI_Send(&total_recv_count, 1, MPI_INT, (world_rank + 1) % world_size, 0, MPI_COMM_WORLD);
        
    }
    MPI_Finalize();

}



