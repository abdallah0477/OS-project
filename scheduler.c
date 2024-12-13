#include "headers.h"

struct msgbuff{
    long mtype;
    struct Process process;
};

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
    int shmNumberProcess = shmget(keyidshmid,sizeof(int),0666 | IPC_CREAT);

    semun.val = 0;
    
    
    semctl(semsend, 0, SETVAL, semun);
    semctl(semrec, 0, SETVAL, semun);

    int *shmaddr = (int *)shmat(shmNumberProcess, (void *)0, 0);
    int N = *shmaddr;
    processes = malloc(N * sizeof(struct Process));
    int process_count = 0;
    printf("in receiving loop\n");
    while (1) {
        if (process_count < N) {  
        down(semsend);  
        
            struct msgbuff processmsg;
            processmsg.mtype = 1;  

            
            if (msgrcv(ProcessQueueid, &processmsg, sizeof(struct Process), 1, IPC_NOWAIT) == -1) {
                perror("Could not receive message bitch");
                exit(1);
            }

        struct Process receivedProcess = processmsg.process;
        printf("Received Process: ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
            receivedProcess.id, receivedProcess.arrival_time, 
            receivedProcess.running_time, receivedProcess.priority);

        processes[process_count] = receivedProcess;
        process_count++;

        up(semrec);  
    }
}
    destroyClk(true);
}
