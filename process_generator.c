#include "headers.h"

void clearResources(int);

int countProcesses(FILE *file) {
    int count = 0;
    char buffer[256];
    
    // Start from beginning of file
    rewind(file);
    
    while (fgets(buffer, sizeof(buffer), file)) {
        // Skip lines that start with # or are empty
        if (buffer[0] == '#' || buffer[0] == '\n') {
            continue;
        }
        
        // Check if line has at least one digit (simple check)
        if (strpbrk(buffer, "0123456789") != NULL) {
            count++;
        }
    }
    
    return count;
}

pid_t clkpid,schedulerpid;
key_t semclkid,semsendid,semrecid,ProcessQueueid,keyidshmid,keyidshmid2;

int shmNumberProcess,semsend,semrec,ProcessQueue;

#define ARRAY_SIZE 3

struct Process *processes;

int* shmaddrinfo;

int main(int argc, char *argv[])
{       
    signal(SIGINT, clearResources);

    // clearResources(SIGINT);
    union Semun semun;
    //Message Queue for Processes
    ProcessQueueid = ftok("process_generator",68);
    ProcessQueue = msgget(ProcessQueueid, 0666 | IPC_CREAT);
    if (ProcessQueue == -1) {
    perror("msgget failed");
    exit(1);
}


    //Shared Memory Array for Process Details
    keyidshmid = ftok("process_generator",69);    
    shmNumberProcess = shmget(keyidshmid,sizeof(int) * ARRAY_SIZE,0666 | IPC_CREAT);
    shmaddrinfo = (int *)shmat(shmNumberProcess, (void *)0, 0); 
    if (shmaddrinfo == (int *)-1) {
        perror("shmat failed");
        exit(1);
    }
    int choice;

    // TODO Initialization
    // 1. Read the input files.
    printf("Choose a scheduling algorithm\n");
    printf("1. HPF (Highest Priority First)\n");
    printf("2. STRN (Shortest time remaining next)\n");
    printf("3. RR (Round Robin)\n");
    scanf("%d", &choice);

    int Quantum;

    if (choice == 1){
        printf("You have selected HPF\n");
    }
    else if (choice == 2){
        printf("You have selected STRN\n");
    }
    else if (choice == 3){
        printf("You have selected RR\n");
        printf("Enter the quantum time: ");
        scanf("%d", &Quantum);
    }
    else {
        printf("Invalid choice\n");
        exit(1);
    }


    FILE *pfile;
    pfile = fopen("process.txt","r");

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
    int N = countProcesses(pfile);
    if (N == 0) {
        printf("No processes found in the file\n");
        fclose(pfile);
        exit(1);
    }    // if (fscanf(pfile, "%d", &N) != 1) {
    //     perror("Error reading file");
    //     fclose(pfile);
    //     return 1;
    // }

    // int Quantum = 1;
    // if(Scheduling_Algorithm == 3 || Scheduling_Algorithm ==4){
    //     Quantum = atoi(argv[5]);
    // }
   int *info = malloc(sizeof(int) * ARRAY_SIZE);
    info[0] =N;
    info[1] = choice;
    info[2]= Quantum;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        shmaddrinfo[i] = info[i];
    }

    printf("Process generator Processes: %d Scheduling algorithm number: %d quantum: %d\n",N,choice,Quantum);

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

    initClk();
    printf(" in Process Generator\n");
    

    // 4. Use this function after creating the clock process to initialize clock.
    
    // To get time use this function. 
    int x = getClk();
    printf("Current Time is %d\n", x);
    // TODO Generation Main Loop
    
    // 5. Create a data structure for processes and provide it with its parameters.
    processes = malloc(N * sizeof(struct Process));
    int process_count = 0;
    char buffer[256];
    rewind(pfile);
    while (fgets(buffer, sizeof(buffer), pfile)) {
        // Skip lines that start with #
        if (buffer[0] == '#') {
            continue;
        }

        // 
        struct Process p;
        if (sscanf(buffer, "%d %d %d %d %d", &p.id, &p.arrival_time, &p.running_time,&p.priority,&p.MEMSIZE) == 5) {
            if (process_count < N) {
                processes[process_count++] = p;
            }
        }
    }
    
   int current_process = 0;

    // Print the processes to verify
    // printf("Processes:\n");
    // for (int i = 0; i < process_count; i++) {
    //     printf("[%d]""ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
    //            i,processes[i].id, processes[i].arrival_time, 
    //            processes[i].running_time, processes[i].priority);
    // }

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

            // printf("Process sent: ID: %d, Arrival: %d\n", 
            //        processes[current_process].id, 
            //        processes[current_process].arrival_time);

            
            current_process++;
        }
        

    }

   

    // 7. Clear clock resources
    
    fclose(pfile);
    waitpid(schedulerpid,NULL,0);
    waitpid(clkpid,NULL,0);
    shmdt(shmaddrinfo);  
    shmctl(shmNumberProcess, IPC_RMID, NULL); 
    msgctl(ProcessQueue, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    return 0;

}

void clearResources(int signum)
{
    // system("pkill -f clk.out");
    // shmdt(shmaddr);
    kill(clkpid,SIGINT);
    kill(clkpid, SIGTERM);
    kill(schedulerpid,SIGTERM);


    
    shmdt(shmaddrinfo);  
    shmctl(shmNumberProcess, IPC_RMID, NULL);

    

    msgctl(ProcessQueue, IPC_RMID, (struct msqid_ds *)0);

    if(processes != NULL){
        free(processes);
    }
    waitpid(clkpid, NULL, 0);
    waitpid(schedulerpid, NULL, 0);
    destroyClk(true);
    exit(0);

}
