/*
	Developer: Alon Kalif
	File: 	   wd.c
	Reviewer:  Shimon
	Date: 	   12.10.2024
	Status:    Approved
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>             /* For getenv */
#include <time.h>               /* For time_t */

#include <sys/ipc.h>            /* For ftok() */
#include <sys/shm.h>            /* For shmget() */

#include <fcntl.h>              /* For O_* constants */
#include <sys/stat.h>           /* For mode constants */

#include <string.h>             /* For memcpy */

#include "wd.h"                 /* For WDStart */

static void ReadFromSharedMem(time_t* interval_in_sec, size_t* intervals_per_check)
{
    key_t key;
    int block_id = 0;
    char* shared_memory = NULL;

    key = ftok("wd.out", 0);
    block_id = shmget(key, sizeof(time_t) + sizeof(size_t), 0);
    shared_memory = shmat(block_id, NULL, 0);
    
    memcpy(interval_in_sec, shared_memory, sizeof(time_t));
    memcpy(intervals_per_check, shared_memory + sizeof(time_t), sizeof(size_t));

    shmdt(shared_memory);
}

int main(int argc, char* argv[])
{ 
    time_t interval_in_sec;
    size_t intervals_per_check;

    ReadFromSharedMem(&interval_in_sec, &intervals_per_check);

    setenv("FRIEND_STATUS", "ALIVE", 1);
    
    printf("wd (child) pid = %d\n", getpid());

    WDStart(interval_in_sec, intervals_per_check, argc, argv);

    return 0;
}