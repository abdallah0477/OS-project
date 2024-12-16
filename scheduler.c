#include "headers.h"
#define ARRAY_SIZE 3



//===============functions managing pauses=================
void start(struct Process* p);
void finish(struct Process p);
void resume(struct Process p);
void Pause(struct Process p);
//==========================================================

char* p_path;// process path to be used in calling the file

//variables for totals
int total_wait=0;
int total_wta=0;
int total_run=0;


FILE *out_log;
FILE *out_perf;

void SJF(int N,int MessageQueue,struct PriQueue *pq);
struct Process *processes;
pid_t ProcessID;
int main(int argc, char *argv[])
{
    initClk();
    printf(" in Scheduler\n");


    char processBuffer[500];
    getcwd(processBuffer, sizeof(processBuffer));//putting the directory path into the buffer
    p_path = strcat(processBuffer, "/process.out");

    //initialize output files-------------
    out_log=fopen("scheduler.log","w");
    out_perf=fopen("scheduler.perf","w");
    //------------------------------------

    union Semun semun;


    key_t ProcessQueueid = ftok("process_generator",68);
    key_t keyidshmid = ftok("process_generator",69);
    key_t runningtimeid = ftok("process_generator",100);
    


    int ProcessQueue = msgget(ProcessQueueid, 0666 | IPC_CREAT);
    if (ProcessQueue == -1) {
    perror("msgget failed");
    exit(1);
}
    int shmNumberProcess = shmget(keyidshmid,sizeof(int) * ARRAY_SIZE,0666 | IPC_CREAT);
    int runningtime = shmget(keyidshmid,sizeof(int),IPC_CREAT);


    semun.val = 0;
    


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
        SJF(N,ProcessQueue,&pq);
        printf("im done\n");
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





// start function definition
void start(struct Process* process) {
    if(process->id <= -1) {
        return;
    }
     int waiting_time=getClk()-process->arrival_time;
     process->wait_time=waiting_time;

    fprintf(out_log, "At time %d process %d started, arrival time %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival_time, process->running_time,
            process->running_time, waiting_time);
    printf("At time %d process %d started, arrival time %d total %d remain %d wait %d\n",
            getClk(), process->id, process->arrival_time, process->running_time,
            process->running_time, waiting_time);
    
    int Pid = fork();
    process->p_pid= Pid;
    if (process->p_pid == 0)
    { 
        char Running_Time[10];
        sprintf(Running_Time,"%d",process->running_time);//convert the remaining time into string to be sended to the created process
        execl(p_path,"process.out",Running_Time,NULL);  
    }


}

//finish function definition
void finish(struct Process process) {

    int finishTime = getClk();
    int TA = finishTime - process.arrival_time;
    double WTA =(double) TA / process.running_time;

    printf("At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process.id, process.arrival_time, process.running_time, process.wait_time, TA, WTA);
    fprintf(out_log, "At time %d process %d finished, arrived %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            finishTime, process.id, process.arrival_time, process.running_time, process.wait_time, TA, WTA);


    total_wait+= process.wait_time;
    total_wta+= WTA;
    total_run += process.running_time;

    kill(process.p_pid, SIGCONT);


}

//pause function definition
void Pause(struct Process process)
{
    process.time_stopped=getClk();
    fprintf(out_log, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
    getClk(), process.id, process.arrival_time, process.running_time,process.remaining_time,process.wait_time);
    printf("At time %d process %d stopped arr %d total %d remain %d wait %d\n",
    getClk(), process.id, process.arrival_time, process.running_time,process.remaining_time,process.wait_time);
}

//resume function definition
void resume(struct Process process)
{

    process.wait_time += getClk()-process.time_stopped;
    fprintf(out_log, "At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
    getClk(), process.id, process.arrival_time, process.running_time,process.remaining_time,process.wait_time);
    printf("At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
    getClk(), process.id, process.arrival_time, process.running_time,process.remaining_time,process.wait_time);

}


void SJF(int N,int ProcessQueue,struct PriQueue* pq){
    int process_count = 0;
    int remaining_time = 0;
    struct Process curr;
    curr.id=-1;
    struct msgbuff processmsg;
    while (process_count < N || !isEmpty(pq) || curr.state ==1) {
        if(!isEmpty(pq) && curr.id == -1){
            curr = dequeue(pq);
            remaining_time = curr.running_time;
        }
        if(curr.state == 0 ){
            curr.state = 1;
            start(&curr);
            sleep(1);
            remaining_time--;
        }
        else{
            sleep(1);
            remaining_time --;
        }   

        if(remaining_time <= 0){
            curr.state = 0;
            finish(curr);
            if(!isEmpty(pq)){
            curr = dequeue(pq);
            }
            else{
                curr.id =-1;
                printf("currid = %d",curr.id);
            }
        }
        while(true){
            if(msgrcv(ProcessQueue, &processmsg, N* sizeof(struct Process), 1, IPC_NOWAIT) == -1)
            break;
            enqueue(pq,processmsg.process,0);
            printf("Schduler Received Process with pid %d\n",processmsg.process.id);
            process_count++;
            printf("imhere\n");
        }
    }
} 