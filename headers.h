#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "string.h"
#include <errno.h>
#define MAX_SIZE 100






void removeSemaphores(key_t semid)
{
    if (semctl(semid, 0, IPC_RMID, NULL) == -1) {
        perror("Error removing semaphore");
    } else {
        printf("Semaphore removed successfully.\n");
    }
}
union Semun
{
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};
void down(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
 
struct Process{
    int id;
    int priority; 
    int arrival_time;
    int running_time;
    int remaining_time;
    int finish_time;
    int turnaroundtime;
    float wta;
    int state;//ready,running,finished
};

struct msgbuff{
    long mtype;
    struct Process process;
};

struct PriQueue {
    struct Process queue[MAX_SIZE];
    int size;
};

int isEmpty(struct PriQueue* pq) {
    return pq->size == 0; 
}

void enqueue(struct PriQueue* pq, struct Process P, int use_priority) {
    if (pq->size >= MAX_SIZE) {
        printf("Full Queue\n");
        return; 
    }

    int i;
    if (use_priority) {
        // Sort by priority (HPF) priority=1
        for (i = pq->size - 1; i >= 0 && pq->queue[i].priority > P.priority; i--) {
            pq->queue[i + 1] = pq->queue[i];
        }
    } else {
        // Sort by running time (SJF) priority=0
        for (i = pq->size - 1; i >= 0 && pq->queue[i].running_time > P.running_time; i--) {
            pq->queue[i + 1] = pq->queue[i];
        }
    }
    pq->queue[i + 1] = P;
    pq->size++;
}

struct Process dequeue(struct PriQueue* pq) {
    if (isEmpty(pq)) {
        printf("Queue Empty\n");
        return ;
    }

    struct Process P = pq->queue[0];
    for (int i = 1; i < pq->size; i++) {
        pq->queue[i - 1] = pq->queue[i];
    }

    pq->size--;
    return P;
}

struct Process peek(struct PriQueue* pq) {
    if (isEmpty(pq)) {
        printf("Queue Empty\n");
        return;
    }

    return pq->queue[0]; 
}



void printPriQueue(struct PriQueue* pq) {
    if (isEmpty(pq)) {
        printf("The queue is empty.\n");
        return;
    }

    printf("Priority Queue Contents:\n");
    for (int i = 0; i < pq->size; i++) {
        printf("Process[%d]: ID: %d, Arrival Time: %d, Running Time: %d, Priority: %d\n",
               i, pq->queue[i].id, pq->queue[i].arrival_time, 
               pq->queue[i].running_time, pq->queue[i].priority);
    }
}

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All processes call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
    printf("Clock Successfully initalized");
}

/*
 * All processes call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
