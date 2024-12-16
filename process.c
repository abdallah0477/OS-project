#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    initClk();
    remainingtime = atoi(argv[1]);
    printf("the remaining time for the created process is %d ...\n",remainingtime);
    int temp = -1; 
    //TODO The process needs to get the remaining time from somewhere
    //remainingtime = ??;
    while (remainingtime > 0)
    {
        while (temp== getClk());
            if (temp != getClk()) {
                remainingtime--;
                temp = getClk();
            }
        printf("im here\n");
    }

    destroyClk(false);

    return 0;
}
