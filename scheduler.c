#include "headers.h"
#define ARRAY_SIZE 3

//===============functions managing pauses=================
void start(struct Process *p);
void finish(struct Process *p);
void resume(struct Process *p);
void Pause(struct Process *p);
//==========================================================

char *p_path; // process path to be used in calling the file

// variables for totals
int total_wait = 0;
int total_wta = 0;
int total_run = 0;
float CPU_UT = 0;
float avg_WTA = 0;
float avg_wait = 0;

int still_sending = 1;
FILE *out_log;
FILE *out_perf;

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
        SJF(N, ProcessQueue, &pq);
    }
    if (Scheduling_Algorithm == 2)
    {
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
        multifeedback(ProcessQueue, N, Quantum);
    }

    avg_wait = total_wait / (float)N;
    float CPU_UT = ((float)total_run / (float)(getClk())) * 100.0;
    avg_WTA = total_wta / (float)N;
    fprintf(out_perf, "CPU Utilization = %.0f%%\nAVG WTA= %f\nAVG Waiting Time= %.2f\n",
            CPU_UT, avg_WTA, avg_wait);
    printf("CPU Utilization = %.0f%%\nAVG WTA= %f\nAVG Waiting Time= %.2f\n",
           CPU_UT, avg_WTA, avg_wait);
    printPriQueue(&pq);

    // printf("Processes:\n");
    // for (int i = 0; i < process_count; i++) {
    //     printf("[%d]""ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
    //            i,processes[i].id, processes[i].arrival_time,
    //            processes[i].running_time, processes[i].priority);
    // }

    if (msgctl(ProcessQueue, IPC_RMID, NULL) == -1)
    {
        perror("Failed to remove message queue");
    }

    shmdt(shmaddr);
    return 0;
    destroyClk(false);
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

    kill(process->p_pid, SIGCONT);
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
    fprintf(out_log, "At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
            getClk(), process->id, process->arrival_time, process->running_time, process->remaining_time, process->wait_time);
    printf("At time %d process %d resumed arr %d total %d remain %.2d wait %.2d\n",
           getClk(), process->id, process->arrival_time, process->running_time, process->remaining_time, process->wait_time);
}

void SJF(int N, int ProcessQueue, struct PriQueue *pq)
{
    int process_count = 0;
    int remaining_time = 0;
    struct Process curr;
    curr.id = -1; // Initialize to indicate no current process
    struct msgbuff processmsg;

    while (process_count <= N || !isEmpty(pq) || curr.state == 1)
    {
        if (!isEmpty(pq) && curr.id == -1)
        {
            curr = dequeue(pq);
            remaining_time = curr.running_time;
        }

        if (curr.id != -1)
        {
            if (curr.state == 0)
            {
                curr.state = 1;
                start(&curr);
            }
            sleep(1);
            remaining_time--;
        }

        if (remaining_time == 0 && curr.id != -1)
        {
            curr.state = 0;
            finish(&curr);

            if (!isEmpty(pq))
            {
                curr = dequeue(pq);
                remaining_time = curr.running_time;
            }
            else
            {
                curr.id = -1;
                printf("Scheduler idle, no current process\n");
            }
        }

        //
        while (true)
        {
            if (msgrcv(ProcessQueue, &processmsg, N * sizeof(struct Process), 1, IPC_NOWAIT) == -1)
            {
                if (errno == ENOMSG)
                {
                    break;
                }
                else
                {
                    perror("msgrcv failed");
                    exit(1);
                }
            }
            enqueue(pq, processmsg.process, 0);
            printf("Scheduler Received Process with pid %d\n", processmsg.process.id);
            process_count++;
        }

        if (process_count >= N && isEmpty(pq) && curr.id == -1)
        {
            break;
        }
    }
}

void hpf(int N, int ProcessQueue, struct PriQueue *pq)
{
    int process_count = 0;
    int remaining_time = 0;
    struct Process curr;
    curr.id = -1; // Initialize to indicate no current process
    struct msgbuff processmsg;

    while (process_count <= N || !isEmpty(pq) || curr.state == 1)
    {
        if (!isEmpty(pq) && curr.id == -1)
        {
            curr = dequeue(pq);
            remaining_time = curr.running_time;
        }

        if (curr.id != -1)
        {
            if (curr.state == 0)
            {
                curr.state = 1;
                start(&curr);
            }
            if (curr.state == 2)
            { // paused
                curr.state = 1;
                resume(&curr);
            }
            sleep(1);
            remaining_time--;
            printf("Decrementing\n\n\n\n");
        }

        struct Process temp = {0};
        temp.state = -1;
        if (processmsg.process.id > 0)
        {
            enqueue(pq, processmsg.process, 1);
        }
        temp = peek(pq);
        if (!isEmpty(pq))
        {
            struct Process temp = peek(pq);
            if (temp.priority < curr.priority)
            { // Lower value = higher priority
                printf("Preempting process %d for process %d\n", curr.id, temp.id);
                curr.state = 2;
                Pause(&curr);
                enqueue(pq, curr, 1);

                curr = dequeue(pq);
                curr.state = 1;
                remaining_time = curr.running_time;
            }
        }

        if (remaining_time == 0 && curr.id != -1)
        {
            curr.state = 0;
            printf("Process with id %d finishied", curr.id);
            finish(&curr);

            if (!isEmpty(pq))
            {
                curr = dequeue(pq);
                remaining_time = curr.running_time;
            }
            else
            {
                curr.id = -1;
                printf("Scheduler idle, no current process\n");
            }
        }

        //
        while (true)
        {
            if (msgrcv(ProcessQueue, &processmsg, N * sizeof(struct Process), 1, IPC_NOWAIT) == -1)
            {
                if (errno == ENOMSG)
                {
                    break;
                }
                else
                {
                    perror("msgrcv failed");
                    exit(1);
                }
            }
            printf("Scheduler Received Process with pid %d\n", processmsg.process.id);
            process_count++;
        
            }
        

        
        if (process_count >= N && isEmpty(pq)) {
            break;
        }
    }
}


void multifeedback(int ProcessQueueid, int n, int q)
{
    struct circularqueue mlfq[10];
    for (int i = 0; i < 10; i++)
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


    

    printf("MLFQ Scheduler started with fixed quantum %d.\n", q);


    while (process_count < n )
    {
        if(process_count==n){
            break;
        }

        while (msgrcv(ProcessQueueid, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1)
        {

            printf("process with id %d priority %d arrived\n", processmsg.process.id,processmsg.process.priority);
            enqueuecircular(&mlfq[processmsg.process.priority], processmsg.process);
            processmsg.process.state=0;

        }

        clock_time = getClk();

       
        if (current_process.id == -1)
        {
            for (int i = 0; i < 10; i++)
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

            while (getClk() < end_time)
            { // Time the process will run
                while (msgrcv(ProcessQueueid, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1)
                {
                    printf("Process with ID %d and priority %d arrived\n", processmsg.process.id, processmsg.process.priority);
                    enqueuecircular(&mlfq[processmsg.process.priority], processmsg.process);

                    if (processmsg.process.priority < current_level)
                    {
                        higher_process = 1;
                        higher_level = processmsg.process.priority;
                    }
                }
            }

            current_process.remaining_time -= exec_time;
            if (current_process.remaining_time > 0)
            {
                Pause(&current_process);
                if(current_level<9){
                    enqueuecircular(&mlfq[current_level + 1], current_process);
                }
                else{
                    enqueuecircular(&mlfq[current_process.priority], current_process);
                }
                current_process.id=-1;
            }
            else
            {
                finish(&current_process);
                process_count++;
                current_process.state=2;
                current_process.id=-1;
            }

           if(process_count==n){
            break;
            }

            if (higher_process == 0 || higher_level == -1)
            {
                while(isEmptyCircular(&mlfq[current_level])){
                    current_level++;
                    if(current_level==10){
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
}




void RoundRobin(int ProcessQueue, int N, int Quantum)
{
    struct circularqueue readyprocesses;
    int process_count;
    struct Process p;
    initialq(&readyprocesses);
    // signal(SIGUSR1, process_finished_handler);
    int executiontime;
    struct msgbuff processmsg;
    int timeslot = getClk();
    if (Quantum <= 0)
    {
        Quantum = 1;
    }
    while (true)
    {

        while (msgrcv(ProcessQueue, &processmsg, sizeof(processmsg.process), 1, IPC_NOWAIT) != -1)
        {
            if (processmsg.process.id != -1)
            {
                enqueuecircular(&readyprocesses, processmsg.process);
                process_count++;
            }
        }

        if (!still_sending && isEmptyCircular(&readyprocesses))
        {
            return;
        }

        // Process a ready process from the queue
        if (!isEmptyCircular(&readyprocesses))
        {
            struct Process p = dequeuecircular(&readyprocesses);

            // If process is not yet run
            if (!p.run_before)
            {
                p.run_before = true;
                start(&p);
                p.remaining_time = p.running_time;
            }
            else
            {
                resume(&p);
            }

            if (p.remaining_time > Quantum)
            {
                executiontime = Quantum;
            }
            else
            {
                executiontime = p.remaining_time; // Finish the process
            }

            int end = getClk() + executiontime;
            while (getClk() < end)
            {
                sleep(1);
            }

            p.remaining_time -= executiontime;
            if (p.remaining_time > 0)
            {
                Pause(&p);
                enqueuecircular(&readyprocesses, p);
            }
            else
            {
                finish(&p);
            }
        }

        if (process_count == N && isEmptyCircular(&readyprocesses))
        {
            break;
        }
    }

}
