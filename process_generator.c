#include "headers.h"

void clearResources(int);
struct Process{
    int priority; 
    int arrival_time;
    int running_time;
};
int main(int argc, char *argv[])
{

    pid_t clkpid,schedulerpid;
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.

    if (argc == 3) {
        if (strcmp(argv[2], "1") == 0) {
            printf("Shortest Job First Scheduling\n");
        } else if (strcmp(argv[2], "2") == 0) {
            printf("Highest Priority First Scheduling\n");
        } else {
            printf("Invalid scheduling algorithm number.\n");
        }
    } 
    else if (argc == 4) {
        if (strcmp(argv[2], "3") == 0) {
            printf("Round Robin Scheduling With Quantum = %s\n", argv[3]);
        } 
        else if (strcmp(argv[2], "4") == 0) {
            printf("Multilevel Feedback Queue Scheduling With Quantum = %s\n", argv[3]);
        } 
        else {
            printf("Invalid scheduling algorithm number.\n");
        }
    } 
    else {
        printf("Invalid number of arguments. Usage: ./scheduler <algorithm_number> [quantum]\n");
    }

    FILE *pfile;
    pfile = fopen("process.txt","r");

    // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.

    int Scheduling_Algorithm = atoi(argv[2]);
    int Quantum =0;
    if(Scheduling_Algorithm == 3 || Scheduling_Algorithm ==4){
        Quantum = atoi(argv[3]);
    }
    printf("Scheduling algorithm number %d, quantum =%d\n",Scheduling_Algorithm,Quantum);

    // 3. Initiate and create the scheduler and clock processes.
    clkpid = fork(); // 3amalt fork le clk proceess
    
    if (clkpid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (clkpid == 0) { // child process
        
        execl("./clk.out", "clk.out", (char *)NULL); 
        
        perror("Error executing clk.out");
        return 1;
    } 

    schedulerpid = fork();
    if (schedulerpid == -1) {
        perror("Fork failed");
        return 1;
    }

    if (schedulerpid == 0) { // child process
        
        execl("./scheduler.out", "scheduler.out", (char *)NULL); 
        perror("Error executing clk.out");
        return 1;
    } 


    // 4. Use this function after creating the clock process to initialize clock.
    initClk();
    // To get time use this function. 
    int x = getClk();
    printf("Current Time is %d\n", x);
    // TODO Generation Main Loop
    
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption

}
