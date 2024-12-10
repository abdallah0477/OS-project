#include "headers.h"

void clearResources(int);

int main(int argc, char *argv[])
{

    pid_t clkpid,schedulerpid;
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
    
    int Scheduling_Algorithm = atoi(argv[3]);
    int Quantum =0;
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

    if (clkpid == 0) { // child process
        
        execl("./clk.out", "clk.out", (char *)NULL); 

    } 
    schedulerpid = fork();
    if (schedulerpid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (schedulerpid == 0) { // child process
        
        execl("./scheduler.out", "scheduler.out", (char *)NULL); 
        perror("Error executing scheduler.out");
        return 1;
    } 

    initClk();
    // 4. Use this function after creating the clock process to initialize clock.
    
    // To get time use this function. 
    int x = getClk();
    printf("Current Time is %d\n", x);
    // TODO Generation Main Loop
    
    // 5. Create a data structure for processes and provide it with its parameters.
    struct Process *processes = malloc(N * sizeof(struct Process));
    int process_count = 0;
    while (fgets(buffer, sizeof(buffer), pfile)) {
        // Skip lines that start with #
        if (buffer[0] == '#') {
            continue;
        }

        // Parse the process data
        struct Process p;
        if (sscanf(buffer, "%d %d %d %d", &p.id, &p.arrival_time, &p.running_time, &p.priority) == 4) {
            if (process_count < N) {
                processes[process_count++] = p;
            }
        }
    }
        fclose(pfile);

    // Print the processes to verify
    printf("Processes:\n");
    for (int i = 0; i < process_count; i++) {
        printf("ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
               processes[i].id, processes[i].arrival_time, 
               processes[i].running_time, processes[i].priority);
    }



    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    exit(0);

}
