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
#define MAX_MEMORY_SIZE 1024  // 2^10 bytes
#define MIN_BLOCK_SIZE 1      // 2^0 bytes





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
//========================================================PROCESS DATA STRUCTURE===============================================================
typedef struct Node {
    int size;               // Size of the block (in bytes)
    int allocated;          // 0 for free, 1 for allocated
    int start_address; // Start address of the memory block 
    int end_address; // End address of the memory block
    struct Node *left;      // Left child (buddy)
    struct Node *right;     // Right child (buddy)
    struct Node *parent;    // Parent node
} Node;

struct Process{
    int start_a;
    int end_a;
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
    Node* Block;
};

struct msgbuff{
    long mtype;
    struct Process process;
};

//=======================================================WAITING QUEUE==========================================================================
struct WaitQueue {
    struct Process waitQueue[MAX_SIZE];
    int size;
};


int isEmptyWaitQueue(struct WaitQueue* q) {
    return q->size == 0;
}


void enqueueWaitQueue(struct WaitQueue* q, struct Process p) {
    if (q->size >= MAX_SIZE) {
        printf("WaitQueue is full. Cannot enqueue process.\n");
        return;
    }

    q->waitQueue[q->size] = p;
    q->size++;
}


struct Process dequeueWaitQueue(struct WaitQueue* q) {
    if (isEmptyWaitQueue(q)) {
        printf("WaitQueue is empty. Cannot dequeue process.\n");
        struct Process emptyProcess = {0}; 
        emptyProcess.id = -1;
        return emptyProcess;
    }

    struct Process p = q->waitQueue[0]; 

    for (int i = 1; i < q->size; i++) {
        q->waitQueue[i - 1] = q->waitQueue[i];
    }

    q->size--; 
    return p;
}


struct Process peekWaitQueue(struct WaitQueue* q) {
    if (isEmptyWaitQueue(q)) {
        printf("WaitQueue is empty. Cannot peek process.\n");
        struct Process emptyProcess = {0}; 
        emptyProcess.id = -1;
        return emptyProcess;
    }

    return q->waitQueue[0];
}
void printWaitQueue(struct WaitQueue* q) {
    if (isEmptyWaitQueue(q)) {
        printf("The WaitQueue is empty.\n");
        return;
    }

    printf("WaitQueue Contents:\n");
    for (int i = 0; i < q->size; i++) {
        printf("Process[%d]: ID: %d, Arrival Time: %d, Running Time: %d, Priority: %d, Memory Size: %d\n",
               i, 
               q->waitQueue[i].id, 
               q->waitQueue[i].arrival_time, 
               q->waitQueue[i].running_time, 
               q->waitQueue[i].priority,
               q->waitQueue[i].MEMSIZE);
    }
}


//============================================================PRIORITY QUEUE======================================================================//
struct PriQueue {
    struct Process priqueue[MAX_SIZE];
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
        for (i = pq->size - 1; i >= 0 && pq->priqueue[i].priority > P.priority; i--) {
            pq->priqueue[i + 1] = pq->priqueue[i];
        }
    } else {
        // Sort by running time (SJF) priority=0
        for (i = pq->size - 1; i >= 0 && pq->priqueue[i].running_time > P.running_time; i--) {
            pq->priqueue[i + 1] = pq->priqueue[i];
        }
    }
    pq->priqueue[i + 1] = P;
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

    struct Process P = pq->priqueue[0];
    for (int i = 1; i < pq->size; i++) {
        pq->priqueue[i - 1] = pq->priqueue[i];
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

    return pq->priqueue[0]; 
}



void printPriQueue(struct PriQueue* pq) {
    if (isEmpty(pq)) {
        printf("The queue is empty.\n");
        return;
    }

    printf("Priority Queue Contents:\n");
    for (int i = 0; i < pq->size; i++) {
        printf("Process[%d]: ID: %d, Arrival Time: %d, Running Time: %d, Priority: %d\n",
               i, pq->priqueue[i].id, pq->priqueue[i].arrival_time, 
               pq->priqueue[i].running_time, pq->priqueue[i].priority);
    }
}

//===============================================================CIRCULAR QUEUE =================================================================
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

//=============================================================MEMORY ALLOCATION TREE==========================================================

int getNearestPowerOfTwo(int num) {//60 64 //120 128
    if (num <= 0) {
        return 1; 
    }
    int power = 1;
    while ((1 << power) < num) { //binary shifting
        power++;
    }
    return 1 << power; 
}


// Function to create a new node
Node* createNode(int size,int start) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->size = size;
    node->allocated = 0;  // Initially free
    node->start_address=start;
    node->end_address=start+size-1;
    node->left = node->right = node->parent = NULL;
    return node;
}

int isNodeSplit(Node* node) {
    if (node == NULL) {
        return 0; // A NULL node is not split
    }
    return (node->left != NULL && node->right != NULL);
}

void splitTree(Node* node) { //split w create no
    if (node->size == MIN_BLOCK_SIZE) {
        return;
    }
    int half_size=node->size/2;

    node->left = createNode(half_size,node->start_address);
    node->right = createNode(half_size ,node->start_address+half_size);
    node->left->parent = node;
    node->right->parent = node;
}

Node* initBuddySystem() { //initialize memory
    Node* root = createNode(MAX_MEMORY_SIZE,0);
    //int MemoryLeft = MAX_MEMORY_SIZE;
    
    return root;
}
// Function to free a block and merge if possible



Node* findFreeBlock(Node* root, int size) {
    if(root == NULL){// error case
        return NULL;
    }
    if(root->allocated == 1){//base case
        return NULL;
    }
    int BestFit = getNearestPowerOfTwo(size); // el size el ana 3ayzo 
    if(root->size == BestFit && !isNodeSplit(root)){ //walahy law el makan monaseb w msh split (occupied) doos
        return root;
    }
    if(!isNodeSplit(root)){//walahy law el makan msh split w enta lesa msh fel best fit doos
        splitTree(root);
    }
    Node* Block = findFreeBlock(root->left,size); //walahy ehna nas yemeen bas seketna shemal
    if(Block == NULL){
        Node* Block = findFreeBlock(root->right,size); //walahy law el shemal magatsh nedkhol yemeen
    }

}



void printTree(Node* root, int level) {

    if (root == NULL) return;

    // Print the right child first (to display the tree from top-down)
    printTree(root->right, level + 1);

    // Print the current node with proper indentation
    for (int i = 0; i < level; i++) {
        printf("    ");  // Indentation for the current level
    }

    // Print the node's size and allocation status
    printf("[Size: %d, Allocated: %d]\n", root->size, root->allocated);

    // Print the left child
    printTree(root->left, level + 1);
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

void PrintProcess(struct Process p){
    printf("%d\t%d\t%d\t\n",p.id,p.arrival_time,p.MEMSIZE);
}
