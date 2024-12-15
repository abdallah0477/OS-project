#include "headers.h"
#define ARRAY_SIZE 3


struct Process *processes;
int main(int argc, char *argv[])
{
    initClk();
    printf(" in Scheduler\n");

    union Semun semun;

    key_t semsendid = ftok("process_generator",66);
    key_t semrecid = ftok("process_generator",67);
    key_t ProcessQueueid = ftok("process_generator",68);
    key_t keyidshmid = ftok("process_generator",69);
    


    int semsend = semget(semsendid,1, 0666 | IPC_CREAT);
    int semrec = semget(semrecid,1, 0666 | IPC_CREAT);
    int ProcessQueue = msgget(ProcessQueueid, 0666 | IPC_CREAT);
    if (ProcessQueue == -1) {
    perror("msgget failed");
    exit(1);
}
    int shmNumberProcess = shmget(keyidshmid,sizeof(int) * ARRAY_SIZE,0666 | IPC_CREAT);
    

    semun.val = 0;
    
    
    semctl(semsend, 0, SETVAL, semun);
    semctl(semrec, 0, SETVAL, semun);

    int *shmaddr = (int *)shmat(shmNumberProcess, (void *)0, 0); 
    
    int *info = malloc(sizeof(int) * ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        info[i] = shmaddr[i];
    }

    int N = info[0];
    int Scheduling_Algorithm = info[1];
    int Quantum = info[2];
    printf("Scheduler\nProcesses: %d Scheduling algorithm number: %d quantum: %d\n",N,Scheduling_Algorithm,Quantum);
    processes = malloc(N * sizeof(struct Process));
    int process_count = 0;

    struct PriQueue pq= {.size =0};

    if (Scheduling_Algorithm == 1){
    while (process_count < N) {
        down(semsend);  

        struct msgbuff processmsg;
        ssize_t received = msgrcv(ProcessQueue, &processmsg, N* sizeof(struct Process), 1, 0);
        
        if (received == -1) {
            perror("Message receive failed");
            exit(1);
        }

        enqueue(&pq,processmsg.process);

        
        
        //processes[process_count] = processmsg.process;
        //
        // printf("Received Process: ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
        //        processes[process_count].id, 
        //        processes[process_count].arrival_time,
        //        processes[process_count].running_time,
        //        processes[process_count].priority);
        //
        process_count++;
        up(semrec);  
        }
    }
    printPriQueue(&pq);


    // printf("Processes:\n");
    // for (int i = 0; i < process_count; i++) {
    //     printf("[%d]""ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
    //            i,processes[i].id, processes[i].arrival_time, 
    //            processes[i].running_time, processes[i].priority);
    // }


    
    if (msgctl(ProcessQueue, IPC_RMID, NULL) == -1) {
        perror("Failed to remove message queue");
    }
    
    shmdt(shmaddr);
    destroyClk(false);
    return 0;
}