#include "headers.h"
#define ARRAY_SIZE 3

//

//===============functions managing pauses=================
void start(struct Process *p);
void finish(struct Process *p);
void resume(struct Process *p);
void Pause(struct Process *p);
//these functions also concurrently handle ".log" file
//==========================================================
void printPerf(int N); 
// handles .perf file
//=========================================================

char *p_path; // process path to be used in calling the file

// variables for totals
int total_wait = 0;
float total_wta = 0;
int total_run = 0;
float CPU_UT = 0;
double avg_WTA = 0;
float avg_wait = 0;
int still_sending = 1;

FILE *out_log;
FILE *out_perf;
FILE *out_memory;

void multifeedback(int MessageQueue, int n, int q);
void SJF(int N, int MessageQueue, struct PriQueue *pq);
void RoundRobin(int MessageQueue, int N, int Quantum);
void hpf(int MessageQueue, int N, struct PriQueue *pq);
struct Process *processes;
pid_t ProcessID;
int main(int argc, char *argv[])
{
    initClk();
    printf(" in Scheduler\n");
    char processBuffer[500];
    getcwd(processBuffer, sizeof(processBuffer)); // putting the directory path into the buffer
    p_path = strcat(processBuffer, "/process.out");

    // initialize output files-------------
    out_log = fopen("scheduler.log", "w");
    out_perf = fopen("scheduler.perf", "w");
    out_memory=fopen("memory.log","w");
    //------------------------------------

    union Semun semun;

    key_t ProcessQueueid = ftok("process_generator", 68);
    key_t keyidshmid = ftok("process_generator", 69);
    key_t runningtimeid = ftok("process_generator", 100);

    int ProcessQueue = msgget(ProcessQueueid, 0666 | IPC_CREAT);
    if (ProcessQueue == -1)
    {
        perror("msgget failed");
        exit(1);
    }
    int shmNumberProcess = shmget(keyidshmid, sizeof(int) * ARRAY_SIZE, 0666 | IPC_CREAT);
    int runningtime = shmget(keyidshmid, sizeof(int), IPC_CREAT);

    semun.val = 0;

    int *shmaddr = (int *)shmat(shmNumberProcess, (void *)0, 0);

    int *info = malloc(sizeof(int) * ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        info[i] = shmaddr[i];
    }

    int N = info[0];
    int Scheduling_Algorithm = info[1];
    int Quantum = info[2];
    printf("Scheduler\nProcesses: %d Scheduling algorithm number: %d quantum: %d\n", N, Scheduling_Algorithm, Quantum);
    processes = malloc(N * sizeof(struct Process));
    int process_count = 0;

    struct PriQueue pq = {.size = 0};
    
    


    if (Scheduling_Algorithm == 1)
    {
        printf("Starting sjf\n");
        SJF(N, ProcessQueue, &pq);
    }
    
    if (Scheduling_Algorithm == 2)
    {
        printf("Starting hpf\n");
        hpf(N, ProcessQueue, &pq);
    }
    if (Scheduling_Algorithm == 3)
    {
        printf("Starting RR\n");
        RoundRobin(ProcessQueue, N, Quantum);
        printf("i am done");
    }
    if (Scheduling_Algorithm == 4)
    {
        printf("Starting multi level feedback\n");
        multifeedback(ProcessQueue, N, Quantum);
    }
    fclose(out_log);
    printPerf(N);
    printPriQueue(&pq);


    if (msgctl(ProcessQueue, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove message queue");
    }

    shmdt(shmaddr);
    return 0;
    destroyClk(false);
}
//allocate function

Node* allocateMemory(Node* root, int size) {
    Node*Block = findFreeBlock(root,size); //call el function el fo2 washoof hatraga3 eh 

    if (Block == NULL) {
        printf("No suitable block found for allocation for block with size %d\n",size);
        return NULL;
    }

    
    Block->allocated = 1;  
    printf("Memory Allocation Successful\n");
    return Block;
}

//Deallocate function

void freeMemory(Node* block) {
    if (block == NULL || block->allocated==0) return;

    block->allocated = 0;
    printf("Freed block of size %d\n", block->size);

    if (block->parent) {
        Node* buddy;
        if (block == block->parent->left) {
            buddy = block->parent->right;
        } else {
            buddy = block->parent->left;
        }

        if (buddy != NULL && buddy->allocated == 0) {
            // Merge the blocks
            printf("Merging block of size %d with buddy of size %d\n", block->size, buddy->size);
            block->parent->left = NULL;
            block->parent->right = NULL;
            free(buddy); // Free the buddy node
            freeMemory(block->parent);  // Recursively call to merge with parent
        }
    } else {
        // If the block is the root node, ensure it's not freed unless it's the only block
        if (block->left == NULL && block->right == NULL) {
            printf("Root block is the only block, not merging further\n");
        }
    }
}

// start function definition
void start(struct Process *process)
{

    process->run_before=1;
   
    if (process->id <= -1)
    {
        return;
    }
    
     int waiting_time=getClk()-process->arrival_time;
     process->wait_time=waiting_time;
     process->remaining_time = process->running_time;
    fprintf(out_log, "At time %d process %d started, arrival time %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival_time, process->running_time,
            process->remaining_time, waiting_time);
    printf("At time %d process %d started, arrival time %d total %d remain %d wait %d\n",
           getClk(), process->id, process->arrival_time, process->running_time,
           process->remaining_time, waiting_time);

    int Pid = fork();
    process->p_pid = Pid;
    if (process->p_pid == 0)
    {
        char Running_Time[10];
        sprintf(Running_Time, "%d", process->running_time); // convert the remaining time into string to be sended to the created process
        execl(p_path, "process.out", Running_Time, NULL);
    }

}

// finish function definition
void finish(struct Process *process)
{

    int finishTime = getClk();
    int TA = finishTime - process->arrival_time;
    double WTA = (double)TA / process->running_time;

    printf("At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
           finishTime, process->id, process->arrival_time, process->running_time, process->wait_time, TA, WTA);
    fprintf(out_log, "At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process->id, process->arrival_time, process->running_time, process->wait_time, TA, WTA);

    total_wait += process->wait_time;
    total_wta += WTA;
    total_run += process->running_time;
    // fill in memory deallocation
    kill(process->p_pid, SIGKILL);
}

// pause function definition
void Pause(struct Process *process)
{
    process->time_stopped = getClk();
    fprintf(out_log, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival_time, process->running_time, process->remaining_time, process->wait_time);
    printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n",
           getClk(), process->id, process->arrival_time, process->running_time, process->remaining_time, process->wait_time);
}

// resume function definition
void resume(struct Process *process)
{

    process->wait_time += getClk() - process->time_stopped;
    fprintf(out_log, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival_time, process->running_time, process->remaining_time, process->wait_time);
    printf("At time %d process %d resumed arr %d total %d remain %d wait %d\n",
           getClk(), process->id, process->arrival_time, process->running_time, process->remaining_time, process->wait_time);
}
//sjf
void SJF(int N, int ProcessQueue, struct PriQueue *pq) {
    union Semun semun;
    // First semaphore (original)
    key_t semcsync = ftok("process_generator", 110);
    if (semcsync == -1) {
        perror("ftok failed");
        exit(1);
    }

    // Create or access first semaphore
    int semsyncid = semget(semcsync, 1, IPC_CREAT | 0666);
    if (semsyncid == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize the first semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization");
        exit(1);
    }

    // Second semaphore (new)
    key_t semcsync2 = ftok("process_generator", 111);  // Different project ID
    if (semcsync2 == -1) {
        perror("ftok failed for second semaphore");
        exit(1);
    }

    // Create or access second semaphore
    int semsyncid2 = semget(semcsync2, 1, IPC_CREAT | 0666);
    if (semsyncid2 == -1) {
        perror("semget failed for second semaphore");
        exit(1);
    }
    
    // Initialize the second semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid2, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization of second semaphore");
        exit(1);
    }

    int process_count = 0;
    int remaining_time = 0;
    struct Process curr;
    curr.id = -1; // Initialize to indicate no current process
    struct msgbuff processmsg;
    Node *root = initBuddySystem();
    struct WaitQueue *Queue = malloc(sizeof(struct WaitQueue));
    Queue->size = 0;

    while (process_count < N || !isEmpty(pq) || curr.id != -1) {
        if (!isEmpty(pq) && curr.id == -1) {
            curr = dequeue(pq);
            remaining_time = curr.running_time;
        }

        if (curr.id != -1) {
            up(semsyncid2);
            if (curr.state == 0) {
                curr.state = 1;
                start(&curr);
                curr.Block =allocateMemory(root,curr.MEMSIZE);
                fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
                        getClk(), curr.Block->size, curr.id, curr.Block->start_address, curr.Block->end_address); 
            }

            down(semsyncid); 
            remaining_time--;
            
        }

        if (remaining_time == 0 && curr.id != -1) {
            curr.state = 0;
            printPriQueue(pq);
            fprintf(out_memory, "At time %d freed %d bytes from process %d from %d to %d\n",
                    getClk(), curr.Block->size, curr.id, curr.Block->start_address, curr.Block->end_address);
            freeMemory(curr.Block);
            finish(&curr);
            // Check the wait queue
            if (!isEmptyWaitQueue(Queue)) {
                struct Process head = peekWaitQueue(Queue);
                PrintProcess(head);
                printf("%d\n",head.MEMSIZE);
                Node *Check = findFreeBlock(root, head.MEMSIZE);
                if (Check != NULL) {
                    head = dequeueWaitQueue(Queue);
                    PrintProcess(head);
                    enqueue(pq, head, 0);
                }
            }

            // Load the next process
            if (!isEmpty(pq)) {
                curr = dequeue(pq);
                remaining_time = curr.running_time;
            } else {
                curr.id = -1;
                printf("Scheduler idle, no current process\n");
            }
        }

        // Receive new processes
        while (true) {
            if (process_count >= N && isEmpty(pq) && curr.id == -1) {
                break;
            }

            if (msgrcv(ProcessQueue, &processmsg, sizeof(struct Process), 1, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    break;
                } else {
                    perror("msgrcv failed");
                    exit(1);
                }
            }

            Node *Block = findFreeBlock(root, processmsg.process.MEMSIZE);
            if (Block == NULL) {
                enqueueWaitQueue(Queue, processmsg.process);
                printf("Added Process to Wait Queue\n");
                printWaitQueue(Queue);
                process_count++;
            } 
            else {
                enqueue(pq, processmsg.process, 0);
                printPriQueue(pq);
                printf("Scheduler Received Process with pid %d\n", processmsg.process.id);
                process_count++;
            }
        }

        if (process_count >= N && isEmpty(pq) && curr.id == -1) {
            break;
        }
        
    }

if (semctl(semsyncid, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the first semaphore");
    exit(1);
    } 


if (semctl(semsyncid2, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the second semaphore");
    exit(1);
} 
    
}
//hpf
void hpf(int N, int ProcessQueue, struct PriQueue *pq) 
{
        union Semun semun;
    // First semaphore (original)
    key_t semcsync = ftok("process_generator", 110);
    if (semcsync == -1) {
        perror("ftok failed");
        exit(1);
    }

    // Create or access first semaphore
    int semsyncid = semget(semcsync, 1, IPC_CREAT | 0666);
    if (semsyncid == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize the first semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization");
        exit(1);
    }

    // Second semaphore (new)
    key_t semcsync2 = ftok("process_generator", 111);  // Different project ID
    if (semcsync2 == -1) {
        perror("ftok failed for second semaphore");
        exit(1);
    }

    // Create or access second semaphore
    int semsyncid2 = semget(semcsync2, 1, IPC_CREAT | 0666);
    if (semsyncid2 == -1) {
        perror("semget failed for second semaphore");
        exit(1);
    }
    
    // Initialize the second semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid2, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization of second semaphore");
        exit(1);
    }
    
    //state=1 started state=2 resumed state=3 stopped state=4 finished 
    int process_count = 0;
    
    struct Process curr;
    curr.id = -1; // Initialize to indicate no current process
    curr.run_before = 0; // Flag to track if the process has run before
    struct msgbuff processmsg;
    Node* root = initBuddySystem(); // Initialize memory management system
    struct WaitQueue* Queue = malloc(sizeof(struct WaitQueue));
    Queue->size = 0;

    while (process_count < N || !isEmpty(pq) || curr.id != -1)
    {
        // mafish process running w fi process fil priority queue
        if (!isEmpty(pq) && curr.id == -1)
        {
            curr = dequeue(pq);
        }

        // fi process now
        if (curr.id != -1)
        { 
            if (curr.run_before == 0) //run before bi zero
            {
                curr.state = 1; //started
                curr.run_before = 1;
                curr.Block=allocateMemory(root,curr.MEMSIZE);
                fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
                        getClk(), curr.Block->size, curr.id, curr.Block->start_address, curr.Block->end_address);
                start(&curr);
            }
            else if (curr.state == 3) //if paused, resume the process
            {
                curr.state = 2; //resumed
                resume(&curr);
            }
            // Synchronize with the clock using semaphores
            sleep(1);
            curr.remaining_time--;

            // process has finished execution
            if (curr.remaining_time == 0)
            {
                printf("Process with id %d finished\n", curr.id);
                finish(&curr);
                curr.state=4; //finished
                fprintf(out_memory, "At time %d freed %d bytes from process %d from %d to %d\n",
                    getClk(), curr.Block->size, curr.id, curr.Block->start_address, curr.Block->end_address);
                freeMemory(curr.Block);
                 if (!isEmptyWaitQueue(Queue)) {
                    struct Process head = peekWaitQueue(Queue);
                    Node* Check = findFreeBlock(root, head.MEMSIZE);
                    if (Check != NULL) {
                        head = dequeueWaitQueue(Queue);
                        enqueue(pq, head, 1); // Add to priority queue
                       // fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
                        //        getClk(), head.Block->size, head.id, head.Block->start_address, head.Block->end_address);
                     }
                 }
                if (!isEmpty(pq))
                {
                    curr = dequeue(pq);
                }
                else
                {
                     curr.id = -1; // Reset current process
                     printf("Scheduler idle, no current process\n");
                }
            }
        }

        // Receive new processes from the message queue
        while (true)
        {
            if (msgrcv(ProcessQueue, &processmsg, N * sizeof(struct Process), 1, IPC_NOWAIT) == -1)
            {
                if (errno == ENOMSG)
                {
                    break; // No new messages, exit the loop
                }
                else
                {
                    perror("msgrcv failed");
                    exit(1); // Exit on unexpected error
                }
            }

            printf("Scheduler received process with id %d\n", processmsg.process.id);
            Node* Block1 = findFreeBlock(root, processmsg.process.MEMSIZE);
            if (Block1 == NULL) {
                enqueueWaitQueue(Queue, processmsg.process); // Add to wait queue if memory is unavailable
                printf("Added process with id=%d to wait queue\n",processmsg.process.id);
                printWaitQueue(Queue);
            } else {
                enqueue(pq, processmsg.process, 1); // Add to priority queue
            }
             process_count++;
        }

        // Check for preemption
        if (!isEmpty(pq) && curr.id!=-1)
        {
            struct Process temp = peek(pq); // begining of priority queue
            if (temp.priority < curr.priority) // Higher priority 
            {
                printf("Preempting process %d for process %d\n", curr.id, temp.id);
                curr.state = 3; // Paused
                Pause(&curr);
                enqueue(pq, curr, 1); // Re-add the paused process to the queue
                curr = dequeue(pq); // Switch to the higher-priority process
            }
        }
    }
    
if (semctl(semsyncid, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the first semaphore");
    exit(1);
    } 


if (semctl(semsyncid2, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the second semaphore");
    exit(1);
} 
}
//multilevel feedback queue  
void multifeedback(int ProcessQueueid, int n, int q)
{
        union Semun semun;
    // First semaphore (original)
    key_t semcsync = ftok("process_generator", 110);
    if (semcsync == -1) {
        perror("ftok failed");
        exit(1);
    }

    // Create or access first semaphore
    int semsyncid = semget(semcsync, 1, IPC_CREAT | 0666);
    if (semsyncid == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize the first semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization");
        exit(1);
    }

    // Second semaphore (new)
    key_t semcsync2 = ftok("process_generator", 111);  // Different project ID
    if (semcsync2 == -1) {
        perror("ftok failed for second semaphore");
        exit(1);
    }

    // Create or access second semaphore
    int semsyncid2 = semget(semcsync2, 1, IPC_CREAT | 0666);
    if (semsyncid2 == -1) {
        perror("semget failed for second semaphore");
        exit(1);
    }
    
    // Initialize the second semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid2, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization of second semaphore");
        exit(1);
    }
    struct circularqueue mlfq[11];
    for (int i = 0; i < 11; i++)
    { // Initialize all queues
        initialq(&mlfq[i]);
    }

    int clock_time = 0;
    struct msgbuff processmsg;
    struct Process current_process = {.id = -1};
    int current_level = -1;
    int process_count = 0;
    int higher_process = 0; // Flag to show if a higher priority process entered during run
    int higher_level = -1;
    Node*root = initBuddySystem();
    struct WaitQueue* Queue = malloc(sizeof(struct WaitQueue));
    Queue->size = 0; 
    

    

    printf("MLFQ Scheduler started with fixed quantum %d.\n", q);


    while (process_count < n )
    {
        if(process_count==n){
            break;
        }

        while (msgrcv(ProcessQueueid, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1)
        {

            printf("process with id %d priority %d arrived\n", processmsg.process.id,processmsg.process.priority);
            Node *temp=allocateMemory(root,processmsg.process.MEMSIZE);
            if(temp==NULL){
                enqueueWaitQueue(Queue,processmsg.process);
                printf("process with id %d entered waiting queue\n",processmsg.process.id);
            }
            else{
                fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
               getClk(), processmsg.process.MEMSIZE, processmsg.process.id,temp->start_address, temp->end_address);
               
               processmsg.process.Block=temp;
               processmsg.process.start_a=temp->start_address;
               processmsg.process.end_a=temp->end_address;  
               enqueuecircular(&mlfq[processmsg.process.priority], processmsg.process);           

               processmsg.process.state=0;
               printf("process with id %d entered ready queue\n",processmsg.process.id);
            }

        }

        clock_time = getClk();

       
        if (current_process.id == -1)
        {
            for (int i = 0; i < 11; i++)
            {
                if (!isEmptyCircular(&mlfq[i])) //badawar ala elprocess elhasha8alha
                {
                    current_process = dequeuecircular(&mlfq[i]);
                    current_level = i;
                    break;
                }
            }
        }

        if (current_process.id != -1)
        {


              if (current_process.run_before == 1)
                {
                    resume(&current_process);
                }
                else
                {
                    current_process.remaining_time=current_process.running_time;
                    current_process.state=1;
                    start(&current_process);
                   
                }
              int exec_time = 0;
            if (current_process.remaining_time < q)
            {
                exec_time = current_process.remaining_time;
            }
            else
            {
                exec_time = q;
            }

            int end_time = clock_time + exec_time;

            while (getClk() < end_time)
            { // Time the process will run
                while (msgrcv(ProcessQueueid, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1)
                {
                    
                    printf("process with id %d priority %d arrived\n", processmsg.process.id, processmsg.process.priority);
                    Node *temp = allocateMemory(root, processmsg.process.MEMSIZE);
                    if (temp == NULL)
                    {
                        enqueueWaitQueue(Queue, processmsg.process);
                        printf("process with id %d entered waiting queue\n", processmsg.process.id);
                    }
                    else
                    {
                        fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
                        getClk(),  processmsg.process.MEMSIZE, processmsg.process.id, temp->start_address, temp->end_address);

                        processmsg.process.Block = temp;
                        processmsg.process.start_a=temp->start_address;
                        processmsg.process.end_a=temp->end_address; 
                        enqueuecircular(&mlfq[processmsg.process.priority], processmsg.process);
                        printf("start adress %d\n",processmsg.process.start_a);
                        printf("end adress %d\n",processmsg.process.end_a); 
                        processmsg.process.state = 0;
                        printf("process with id %d entered ready queue\n", processmsg.process.id);

                        if (processmsg.process.priority < current_level)
                        {
                            higher_process = 1;
                            higher_level = processmsg.process.priority;
                        }
                    }
                }
            }
                current_process.remaining_time -= exec_time;
                if (current_process.remaining_time > 0)
                {
                    Pause(&current_process);
                    if (current_level < 10)
                    {
                        enqueuecircular(&mlfq[current_level + 1], current_process);
                    }
                    else{
                    enqueuecircular(&mlfq[current_process.priority], current_process);
                }
                current_process.id=-1;
                }
            else
            {
                
                Node *temp=current_process.Block;
                printf("current time %d \n",getClk());
                printf("size %d\n",current_process.MEMSIZE);
                printf("id %d\n",current_process.id);
                printf("start adress %d\n",current_process.start_a);
                printf("end adress %d\n",current_process.end_a);
                finish(&current_process);

                fprintf(out_memory, "At time %d freed %d bytes from process %d from %d to %d\n",getClk(), current_process.MEMSIZE,current_process.id ,current_process.Block->start_address, current_process.Block->end_address);
                freeMemory(current_process.Block);
                process_count++;
                current_process.state=2;
                current_process.id=-1;
                if(!isEmptyWaitQueue(Queue)){
                struct Process head = peekWaitQueue(Queue);
                Node* Check = allocateMemory(root,head.MEMSIZE);
                if (Check != NULL)
                {
                    head = dequeueWaitQueue(Queue);
                    head.Block=Check;
                    enqueuecircular(&mlfq[head.priority],head);
                    fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
                     getClk(), head.Block->size,head.id, head.Block->start_address, head.Block->end_address);
                }
                }
            }

           if(process_count==n){
            break;
            }

            if (higher_process == 0 || higher_level == -1)
            {
                while(isEmptyCircular(&mlfq[current_level])){
                    current_level++;
                    if(current_level==11){
                        current_level=0;
                    }
                }
                printf("CURRENT LEVEL IS %d\n",current_level);
                current_process = dequeuecircular(&mlfq[current_level]);
            }
            else
            { // Check if a higher priority process arrived
                current_process = dequeuecircular(&mlfq[higher_level]);
                current_level = higher_level;
                higher_level = -1;
                higher_process = 0;
            }
            
    }
    }
    
if (semctl(semsyncid, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the first semaphore");
    exit(1);
    } 


if (semctl(semsyncid2, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the second semaphore");
    exit(1);
} 
}
//round robin
void RoundRobin(int ProcessQueue,int N,int Quantum){

        union Semun semun;
    // First semaphore (original)
    key_t semcsync = ftok("process_generator", 110);
    if (semcsync == -1) {
        perror("ftok failed");
        exit(1);
    }

    // Create or access first semaphore
    int semsyncid = semget(semcsync, 1, IPC_CREAT | 0666);
    if (semsyncid == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize the first semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization");
        exit(1);
    }

    // Second semaphore (new)
    key_t semcsync2 = ftok("process_generator", 111);  // Different project ID
    if (semcsync2 == -1) {
        perror("ftok failed for second semaphore");
        exit(1);
    }

    // Create or access second semaphore
    int semsyncid2 = semget(semcsync2, 1, IPC_CREAT | 0666);
    if (semsyncid2 == -1) {
        perror("semget failed for second semaphore");
        exit(1);
    }
    
    // Initialize the second semaphore to 0
    semun.val = 0;
    if (semctl(semsyncid2, 0, SETVAL, semun) == -1) {
        perror("semctl failed during initialization of second semaphore");
        exit(1);
    }

    struct circularqueue readyprocesses;
    int process_count=0;
    int processes_done=0;
    struct Process p;
    p.run_before=false;
    initialq(&readyprocesses);
    //signal(SIGUSR1, process_finished_handler);
    int executiontime;
    p.id=-1;
    struct msgbuff processmsg;
    Node*root = initBuddySystem();
    struct WaitQueue* Queue = malloc(sizeof(struct WaitQueue));
    Queue->size = 0; 
    int timeslot=getClk();
    if(Quantum<=0){
        Quantum=1;
    }
    printf("RR Scheduler started with fixed quantum %d.\n", Quantum);
     while (true) {
    
    while (msgrcv(ProcessQueue, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1) {
        if (processmsg.process.id != -1) { 
            
            printf("process with id %d  arrived\n", processmsg.process.id);
            Node *temp=allocateMemory(root,processmsg.process.MEMSIZE);
            if(temp==NULL){
                enqueueWaitQueue(Queue,processmsg.process);
                printf("process with id %d entered waiting queue\n",processmsg.process.id);
                printWaitQueue(Queue);
            }
            else{
                 processmsg.process.Block=temp;
                  enqueuecircular(&readyprocesses, processmsg.process);
               fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
               getClk(), temp->size, processmsg.process.id,temp->start_address, temp->end_address);
               printf("process with id %d entered ready queue\n",processmsg.process.id);
            }
            process_count++;
        }
    }    
 if (!still_sending && isEmptyCircular(&readyprocesses) && isEmptyWaitQueue(Queue)) {
        return;
    }

    // Process a ready process from the queue
    if (!isEmptyCircular(&readyprocesses)) {
         p=dequeuecircular(&readyprocesses);
       // If process is not yet run
        if (!p.run_before) {
            p.run_before = true;
            start(&p);
            p.remaining_time=p.running_time;
            
            
        } else {
            resume(&p);
        }
         if (p.remaining_time> Quantum) {
            executiontime = Quantum;
        } else {
            executiontime = p.remaining_time; // Finish the process
        }

        int end = getClk() + executiontime;
     while(getClk()<end){
         sleep(1);
        while (msgrcv(ProcessQueue, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1) {
        if (processmsg.process.id != -1) { 
          printf("process with id %d  arrived\n", processmsg.process.id);
            Node *temp=allocateMemory(root,processmsg.process.MEMSIZE);
            if(temp==NULL){
                enqueueWaitQueue(Queue,processmsg.process);
                printf("process with id %d entered waiting queue\n",processmsg.process.id);
                printWaitQueue(Queue);
            }
            else{
                 processmsg.process.Block=temp;
                 enqueuecircular(&readyprocesses, processmsg.process);
               fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
               getClk(), temp->size, processmsg.process.id,temp->start_address, temp->end_address);
               printf("process with id %d entered ready queue\n",processmsg.process.id);
            }
            process_count++;
        }
    } 
}
      
        p.remaining_time -= executiontime;

        if (p.remaining_time > 0) {
            Pause(&p); 
            //check again before enqueueing for newly arrived processes
            while (msgrcv(ProcessQueue, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1) {
        if (processmsg.process.id != -1) { 
             printf("process with id %d  arrived\n", processmsg.process.id);
            Node *temp=allocateMemory(root,processmsg.process.MEMSIZE);
            if(temp==NULL){
                enqueueWaitQueue(Queue,processmsg.process);
                printf("process with id %d entered waiting queue\n",processmsg.process.id);
                printWaitQueue(Queue);
            }
            else{
                 processmsg.process.Block=temp;
                  enqueuecircular(&readyprocesses, processmsg.process);
               fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
               getClk(), temp->size, processmsg.process.id,temp->start_address, temp->end_address);
               printf("process with id %d entered ready queue\n",processmsg.process.id);
            }
            process_count++;
        }
    } 
        //re_enqueue 
     enqueuecircular(&readyprocesses, p);
        } else {
            printf("Process with id %d finished\n", p.id);
            finish(&p); 
            processes_done++;
            if(processes_done==process_count && processes_done!=0){
                break;
            }
            fprintf(out_memory, "At time %d freed %d bytes from process %d from %d to %d\n",getClk(), p.Block->size, p.id, p.Block->start_address, p.Block->end_address);
             freeMemory(p.Block);
                 if (!isEmptyWaitQueue(Queue)) {
                    struct Process head = peekWaitQueue(Queue);
                    Node* Check = allocateMemory(root, head.MEMSIZE);
                    if (Check != NULL) {
                        head = dequeueWaitQueue(Queue);
                         enqueuecircular(&readyprocesses,head);
                         head.Block=Check;
                        fprintf(out_memory, "At time %d allocated %d bytes for process %d from %d to %d\n",
                                getClk(), head.Block->size, head.id, head.Block->start_address, head.Block->end_address);
                     }
                 }

            }
    }
     if(processes_done==process_count && processes_done!=0){
                break;
            }
    if(process_count == N  && isEmptyCircular(&readyprocesses) && isEmptyWaitQueue(Queue)){
        break;
    }
    }
    
if (semctl(semsyncid, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the first semaphore");
    exit(1);
    } 


if (semctl(semsyncid2, 0, IPC_RMID) == -1) {
    perror("Failed to destroy the second semaphore");
    exit(1);
} 
}
void printPerf(int N){
    avg_wait = total_wait / (float)N;
    float CPU_UT = ((float)total_run / (float)(getClk())) * 100.0;
    avg_WTA = total_wta / (float)N;
    double roundedavg_WTA = ceil(avg_WTA* 100) / 100.0;
    fprintf(out_perf, "CPU Utilization = %.0f%%\nAVG WTA= %.2ff\nAVG Waiting Time= %.1f\n",
            CPU_UT, roundedavg_WTA, avg_wait);
    printf("CPU Utilization = %.0f%%\nAVG WTA= %.2f\nAVG Waiting Time= %.1f\n",
           CPU_UT, avg_WTA, avg_wait);
    fclose(out_perf);
    }