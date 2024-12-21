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
    struct Node *left;      // Left child (buddy)
    struct Node *right;     // Right child (buddy)
    struct Node *parent;    // Parent node
} Node;

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


// Function to create a new node
Node* createNode(int size) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->size = size;
    node->allocated = 0;  // Initially free
    node->left = node->right = node->parent = NULL;
    return node;
}
// Function to split the tree into smaller blocks (recursively)
void splitTree(Node* node) {
    // Stop if the node size is the minimum block size
    if (node->size == MIN_BLOCK_SIZE) {
        return;
    }

    // Create left and right children for the current node
    node->left = createNode(node->size / 2);
    node->right = createNode(node->size / 2);
    node->left->parent = node;
    node->right->parent = node;
    printf("Split from %d to %d\n",node->size,node->left->size);
}

// Function to initialize the memory tree (pre-split buddy system)
Node* initBuddySystem() {
    // Create the root node (1024 bytes)
    Node* root = createNode(MAX_MEMORY_SIZE);

    
    return root;
}

//lesa el etnen functions dol 3ayzeen yetzabaato
// Function to find a free block of at least the given size
Node* findFreeBlock(Node* root, int size) {
    if (root == NULL) {
        return NULL;
        }

    if (root->size == MIN_BLOCK_SIZE) {

        return NULL;
    }
    if (root->size>size) 
    {
    splitTree(root);
    Node* leftResult = findFreeBlock(root->left, size);
    if (leftResult) return leftResult;
    
    Node* rightResult = findFreeBlock(root->right, size);
    return rightResult;
    }
    if(size >= root->size){
        root=root->parent;
        printf("Allocated block of size %d for process with size %d\n",root->size,size);
        return root;
    }
}

// Function to allocate memory (find free block and allocate it)
Node* allocateMemory(Node* root, int size) {
    Node* block = findFreeBlock(root, size);
    if (block == NULL) {
        printf("No suitable block found for allocation\n");
        return NULL;
    }
    
    block->allocated = 1;  // Mark the block as allocated
    return block;
}

// Function to free a block and merge if possible
void freeMemory(Node* block) {
    if (block == NULL) return;

    block->allocated = 0;
    printf("Freed block of size %d\n", block->size);

    // Try to merge with its buddy if both are free
    if (block->parent) {
        Node* buddy; //bashoof right node wala left node;
        if (block == block->parent->left) {
            buddy = block->parent->right;
        } else {
            buddy = block->parent->left;
        }

        if (buddy!=NULL && buddy->allocated == 0) {
            // Merge the blocks
            printf("Merging block of size %d with buddy of size %d\n", block->size, buddy->size);
            block->parent->left = NULL;
            buddy->parent->right = NULL;
            freeMemory(block->parent);  // Free and merge up the tree
        }
    }
}



// Function to print the structure of the tree (for debugging)
void printTree(Node* root, int level, int isLeft) {
    if (root == NULL) {
        return;
    }

    // Print the tree starting from the right subtree to ensure the root is at the top
    printTree(root->right, level + 1, 0);

    // Print the current node with arrows connecting to parent-child
    for (int i = 0; i < level; i++) {
        printf("       ");  // Indentation for the current level
    }

    if (level > 0) {
        if (isLeft) {
            printf("/-- ");
        } else {
            printf("\\-- ");
        }
    }

    // Print the current node information
    printf("[Size: %d, Allocated: %d]\n", root->size, root->allocated);

    // Print the left subtree
    printTree(root->left, level + 1, 1);
}


//memory allocation

// struct BuddyBlock {
//     int size; // Size of block
//     void* address; // Pointer to the memory block
// };

// struct BuddyAllocator { //akeno struct kebir gowa arrays for each size
//     struct BuddyBlock free_space_array[10]; 
//     //3lshan max size 1024 yebaa ehna mehtageen bss 2^0 lehad 2^9
// };
// char memory_pool[TOTAL_MEMORY_SIZE]; // pointer to the first byte of the array.

// void initialize_buddy_allocator(struct BuddyAllocator* allocator) {
//     //initializing elba2y bi null
//     for (int i = 0; i < MAX_BLOCKS; i++) {
//         allocator->free_space_array[i].size = 0;
//         allocator->free_space_array[i].address = NULL;
//     }
//     // Adding the full memory block (1024 bytes) to the largest block size free list
//     allocator->free_space_array[9].size = MAX_BLOCK_SIZE;
//     allocator->free_space_array[9].address = memory_pool;
    
//     printf("buddy allocator initialized successfully\n");
// }
// void* allocate_memory(struct BuddyAllocator* allocator, int process_size) {
//     int required_size = 1;
//     int index = 0;
    
//     // Find the smallest power of 2 >= size
//     while (required_size < process_size) {
//         required_size *= 2;
//         index++;
//     }
//     printf("Requesting allocation for size: %d\n", process_size);
//     // Check if a block of the required size is available
//     for (int i = index; i < 10; i++) {
//         if (allocator->free_space_array[i].size > 0) {// free list at a particular index i contains a block that is free and large enough for the requested allocation
//             // Split blocks if necessary
//             printf("Found available block of size %d at index %d\n",allocator->free_space_array[i].size, i);
//             while (i > index) {
//                 i--;
//                 allocator->free_space_array[i].size = required_size; //updates freelist
//                 allocator->free_space_array[i].address =
//                     (char*)allocator->free_space_array[i + 1].address + required_size;
//             }

//             // Allocate the block
//             void* block_address = allocator->free_space_array[i].address;
//             allocator->free_space_array[i].size = 0; // Mark block as used
//             printf("Memory allocated successfuly\n");
//             return block_address;
//         }
//     }
//      // No suitable block available
//     printf("Memory allocation failed for requested size: %d\n", process_size);
//     return NULL;
// }
// // Function to print the Buddy Memory Allocation state
// void printBuddyMemory(struct BuddyAllocator* allocator) {
//     printf("\nBuddy Memory Allocation State:\n");
//     printf("-----------------------------------\n");
//     printf("| Index | Block Size | Status      | Address      |\n");
//     printf("-----------------------------------\n");

//     for (int i = 0; i < 10; i++) {
//         if (allocator->free_space_array[i].address != NULL) {
//             printf("| %-5d | %-10d | %-10s | %p |\n",
//                    i, 
//                    allocator->free_space_array[i].size,
//                    allocator->free_space_array[i].size > 0 ? "Free" : "Allocated",
//                    allocator->free_space_array[i].address);
//         } else {
//             printf("| %-5d | %-10d | %-10s | %-12s |\n",
//                    i,
//                    0,
//                    "Unused",
//                    "N/A");
//         }
//     }
//     printf("-----------------------------------\n");
// }





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
