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
#include <math.h>
#define MAX_SIZE 100
#define MAX_BLOCKS 10
#define MAX_BLOCK_SIZE 1024
#define TOTAL_MEMORY_SIZE 1024





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
    int p_pid;
    int id;
    int priority; 
    int arrival_time;
    int running_time;
    int MEMSIZE;
    int remaining_time;
    int finish_time;
    int turnaroundtime;
    float wta;
    int state;//ready,running,finished
    int wait_time;
    int time_stopped;
    int run_before;
    void* memory_address; // Allocated memory block address
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
        printf("Peek Empty qu");
        printf("Dequeue Queue Empty\n");
        struct Process EmptyProcess= {0};
        EmptyProcess.id =-1;
        return EmptyProcess;
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
        struct Process EmptyProcess = {0};
        EmptyProcess.id =-1;
        return EmptyProcess;
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


//circular queue implementation 
struct circularqueue{
    struct Process queuearray[MAX_SIZE];
    int front;
    int rear;
    int size;

};
void initialq(struct circularqueue *cq){
    cq->front=0;
    cq->rear=-1;
    cq->size=0;
}
void enqueuecircular(struct circularqueue *cq , struct Process p){
    if((cq->size>=MAX_SIZE))
    {
        printf("Full Queue");
        return;
    }
    cq->rear=(cq->rear +1)%MAX_SIZE;
    cq->queuearray[cq->rear]=p;
    cq->size++;
}
struct Process dequeuecircular(struct circularqueue *cq){
    if (cq->size==0){
        printf("Empty queue");
        struct Process circularqueue = {0};
        circularqueue.id = -1;
        return circularqueue;
    }
    struct Process p=cq->queuearray[cq->front];
    cq->front=(cq->front +1)%MAX_SIZE;
    cq->size--;
    return p;
}

void printCircularQueue(struct circularqueue *cq) {
    if (cq->size == 0) {
        printf("Circular queue is empty.\n");
        return;
    }

    printf("Circular Queue contents:\n");
    int index = cq->front;
    for (int i = 0; i < cq->size; i++) {
        struct Process p = cq->queuearray[index];
        printf("Process ID: %d, Priority: %d, Remaining Time: %d, State: %d\n", 
               p.id, p.priority, p.remaining_time, p.state);
        index = (index + 1) % MAX_SIZE; 
    }
}

int isEmptyCircular(struct circularqueue *cq) {
    return cq->size == 0;
}


//memory allocation

struct BuddyBlock {
    int size; // Size of block
    void* address; // Pointer to the memory block
};

struct BuddyAllocator { //akeno struct kebir gowa arrays for each size
    struct BuddyBlock free_space_array[10]; 
    //3lshan max size 1024 yebaa ehna mehtageen bss 2^0 lehad 2^9
};
char memory_pool[TOTAL_MEMORY_SIZE]; // pointer to the first byte of the array.

void initialize_buddy_allocator(struct BuddyAllocator* allocator) {
    //initializing elba2y bi null
    for (int i = 0; i < MAX_BLOCKS; i++) {
        allocator->free_space_array[i].size = 0;
        allocator->free_space_array[i].address = NULL;
    }
    // Adding the full memory block (1024 bytes) to the largest block size free list
    allocator->free_space_array[9].size = MAX_BLOCK_SIZE;
    allocator->free_space_array[9].address = memory_pool;
    
    printf("buddy allocator initialized successfully\n");
}
void* allocate_memory(struct BuddyAllocator* allocator, int process_size) {
    int required_size = 1;
    int index = 0;
    
    // Find the smallest power of 2 >= size
    while (required_size < process_size) {
        required_size *= 2;
        index++;
    }
    printf("Requesting allocation for size: %d\n", process_size);
    // Check if a block of the required size is available
    for (int i = index; i < 10; i++) {
        if (allocator->free_space_array[i].size > 0) {// free list at a particular index i contains a block that is free and large enough for the requested allocation
            // Split blocks if necessary
            printf("Found available block of size %d at index %d\n",allocator->free_space_array[i].size, i);
            while (i > index) {
                i--;
                allocator->free_space_array[i].size = required_size; //updates freelist
                allocator->free_space_array[i].address =
                    (char*)allocator->free_space_array[i + 1].address + required_size;
            }

            // Allocate the block
            void* block_address = allocator->free_space_array[i].address;
            allocator->free_space_array[i].size = 0; // Mark block as used
            printf("Memory allocated successfuly\n");
            return block_address;
        }
    }
     // No suitable block available
    printf("Memory allocation failed for requested size: %d\n", process_size);
    return NULL;
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
    printf("Clock Successfully initalized\n");
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
