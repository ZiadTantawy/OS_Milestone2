#include "pcb.h"
#include "queue.h"
#include "interpreter.h"
#include "memory.h"
#include "scheduler.h"
#include <stdio.h>

void scheduleRR()
{
    while (!isEmpty(&readyQueue) || runningPCB.pid != 0)
    {
        scheduleRR_OneStep();
    }
}

// Global static for one-step logic

void scheduleRR_OneStep()
{
    checkDelayedQueue();
    if (runningPCB.pid == 0 || runningPCB.state == TERMINATED || runningPCB.state == BLOCKED)
    {
        if (!isEmpty(&readyQueue))
        {
            runningPCB = dequeue(&readyQueue);
            runningPCB.state = RUNNING;
            rrTimeSliceCounter = 0;
            printf("RR: Running process %d\n", runningPCB.pid);
        }
        else
        {
            printf("RR: No process is ready\n");
            return;
        }
    }

    interpret(&runningPCB, runningPCB.memoryEnd + 1); // Executes one instruction
    rrTimeSliceCounter++;

    if (runningPCB.state == TERMINATED || runningPCB.state == BLOCKED)
    {
        printf("RR: Process %d terminated or blocked\n", runningPCB.pid);
        runningPCB.pid = 0; // Free to run next
        rrTimeSliceCounter = 0;
    }
    else if (rrTimeSliceCounter >= quantumNumber)
    {
        printf("RR: Time quantum expired for PID %d, re-queuing\n", runningPCB.pid);
        runningPCB.state = READY;
        updateState(&runningPCB, READY);
        enqueue(&readyQueue, runningPCB);
        runningPCB.pid = 0;
        rrTimeSliceCounter = 0;
    }
}
