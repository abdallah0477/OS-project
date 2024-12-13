#include "headers.h"

void clearResources(int);

struct msgbuff{
    long mtype;
    struct Process process;
};

pid_t clkpid,schedulerpid;
key_t semclkid,semsendid,semrecid,ProcessQueueid,keyidshmid;



struct Process *processes;



int main(int argc, char *argv[])
{
    
    union Semun semun;
    semclkid = ftok("process_generator",65);
    semsendid = ftok("process_generator",66);
    semrecid = ftok("process_generator",67);
    ProcessQueueid = ftok("process_generator",68);
    keyidshmid = ftok("process_generator",69);

    int semclk = semget(semclkid,1, 0666 | IPC_CREAT);
    int semsend = semget(semsendid,1, 0666 | IPC_CREAT);
    int semrec = semget(semrecid,1, 0666 | IPC_CREAT);
    int ProcessQueue = msgget(ProcessQueueid, 0666 | IPC_CREAT);
    if (ProcessQueue == -1) {
    perror("msgget failed");
    exit(1);
}
    int shmNumberProcess = shmget(keyidshmid,sizeof(int),0666 | IPC_CREAT);

    semun.val = 0;

    semctl(semclk, 0, SETVAL, semun);
    semctl(semsend, 0, SETVAL, semun);
    semctl(semrec, 0, SETVAL, semun);
    int *shmaddr = (int *)shmat(shmNumberProcess, (void *)0, 0); 

    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.

    if (argc == 4) {
        if (strcmp(argv[3], "1") == 0) {
            printf("Shortest Job First Scheduling\n");
        } else if (strcmp(argv[3], "2") == 0) {
            printf("Highest Priority First Scheduling\n");
        } else {
            printf("Invalid scheduling algorithm number.\n");
        }
    } 
    else if (argc == 5) {
        if (strcmp(argv[3], "3") == 0) {
            printf("Round Robin Scheduling With Quantum = %s\n", argv[5]);
        } 
        else if (strcmp(argv[3], "4") == 0) {
            printf("Multilevel Feedback Queue Scheduling With Quantum = %s\n", argv[5]);
        } 
        else {
            printf("Invalid scheduling algorithm number.\n");
        }
    }


    FILE *pfile;
    pfile = fopen("process.txt","r");

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
   int N;
  
   char buffer[256];
    if (fscanf(pfile, "%d", &N) != 1) {
        perror("Error reading file");
        fclose(pfile);
        return 1;
    }
    *shmaddr = N;
    int Scheduling_Algorithm = atoi(argv[3]);
    int Quantum = 1;
    if(Scheduling_Algorithm == 3 || Scheduling_Algorithm ==4){
        Quantum = atoi(argv[5]);
    }
    printf("Scheduling algorithm number %d, quantum = %d\n",Scheduling_Algorithm,Quantum);

    // 3. Initiate and create the scheduler and clock processes.
    clkpid = fork(); // 3amalt fork le clk proceess
    
    if (clkpid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (clkpid == 0) { 
        if (execl("./clk.out", "clk.out", (char *)NULL) == -1) {
            perror("Error executing clk.out"); 
            exit(EXIT_FAILURE); 
            
        }
    }
    up(semclk);
    schedulerpid = fork();

    if (schedulerpid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (schedulerpid == 0) { 
        
        execl("./scheduler.out", "scheduler.out", (char *)NULL); 
        perror("Error executing scheduler.out");
        return 1;
    } 

    while(1){
        down(semclk);
        initClk();
        printf(" in Process Generator\n");
        break;
    }

    // 4. Use this function after creating the clock process to initialize clock.
    
    // To get time use this function. 
    int x = getClk();
    printf("Current Time is %d\n", x);
    // TODO Generation Main Loop
    
    // 5. Create a data structure for processes and provide it with its parameters.
    processes = malloc(N * sizeof(struct Process));
    int process_count = 0;
    while (fgets(buffer, sizeof(buffer), pfile)) {
        // Skip lines that start with #
        if (buffer[0] == '#') {
            continue;
        }

        // 
        struct Process p;
        if (sscanf(buffer, "%d %d %d %d", &p.id, &p.arrival_time, &p.running_time, &p.priority) == 4) {
            if (process_count < N) {
                processes[process_count++] = p;
            }
        }
    }
    
   int current_process = 0;

    // Print the processes to verify
    printf("Processes:\n");
    for (int i = 0; i < process_count; i++) {
        printf("[%d]""ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
               i,processes[i].id, processes[i].arrival_time, 
               processes[i].running_time, processes[i].priority);
    }

    struct msqid_ds queue_info;
    msgctl(ProcessQueue, IPC_STAT, &queue_info);
    if (queue_info.msg_qnum > 0) {
        msgctl(ProcessQueue, IPC_RMID, NULL);
        ProcessQueue = msgget(ProcessQueueid, 0666 | IPC_CREAT);
    }
 // 6. Send the information to the scheduler at the appropriate time.
     while (current_process < N) {
        int current_time = getClk();
        
        if (processes[current_process].arrival_time <= current_time) {
            struct msgbuff processmsg;
            processmsg.mtype = 1;
            processmsg.process = processes[current_process];

            
            if (msgsnd(ProcessQueue, &processmsg, sizeof(struct Process), 0) == -1) {
                perror("Message send failed");
                exit(1);
            }

            printf("Process sent: ID: %d, Arrival: %d\n", 
                   processes[current_process].id, 
                   processes[current_process].arrival_time);

            up(semsend);  
            down(semrec); 
            
            current_process++;
        }
        
        usleep(1000);
    }

   

    // 7. Clear clock resources
    waitpid(schedulerpid,NULL,0);
    fclose(pfile);
    free(processes);
    shmdt(&keyidshmid);
    shmctl(keyidshmid, IPC_RMID, NULL);
    destroyClk(true);
    return 0;

}

void clearResources(int signum)
{
    kill(clkpid, SIGTERM);
    kill(schedulerpid,SIGTERM);

    waitpid(clkpid, NULL, 0);
    waitpid(schedulerpid, NULL, 0);

    shmdt(&keyidshmid);
    shmctl(keyidshmid, IPC_RMID, NULL);


    msgctl(ProcessQueueid, IPC_RMID, (struct msqid_ds *)0);

    if(processes != NULL){
        free(processes);
    }
    
    exit(0);

}
