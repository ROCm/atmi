#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "atmi.h"

/*  Define three PIFS prototypes and their associated kernels */
extern void startKernel(atmi_task_t*thisTask)
   __attribute__((atmi_kernel("start","CPU")));

__kernel void racerKernel(ATMI_myTask, const int myNumber,
    __global int*winner, __global int*loser, __global int*finishers)
    __attribute__((atmi_kernel("racer","GPU")));

extern void finishKernel(atmi_task_t*thisTask, const int*winner, 
   const int*loser, const int*finishers) 
   __attribute__((atmi_kernel("finish","CPU")));

/*  Define two CPU kernels, GPU kernel is defined in racers.cl */
extern  void startKernel(atmi_task_t*thisTask ){
}

extern  void finishKernel(atmi_task_t*thisTask, const int*winner, 
   const int*loser, const int*finishers) {
   printf("Winner is %d\n",*winner);
   printf("Loser is  %d\n",*loser);
   printf("Finishers are %d %d %d\n",finishers[0],finishers[1],finishers[2]);
}

int main(int argc, char **argv) {

   int winner=0,loser=0,finishers[3]={0,0,0};
   ATMI_LPARM(start_lp); ATMI_LPARM(racer_lp);ATMI_LPARM(finish_lp);
   atmi_task_t* racers[3];

   racer_lp->WORKITEMS    = 64; /* Racers are GPUs */
   racer_lp->num_required = 1;
   racer_lp->requires[0]  = start(start_lp); 

   /*  Dispatch the three racer tasks */
   racers[0] = racer(racer_lp,1,&winner,&loser,finishers);
   racers[1] = racer(racer_lp,2,&winner,&loser,finishers);
   racers[2] = racer(racer_lp,3,&winner,&loser,finishers);
   
   finish_lp->num_required = 3;
   finish_lp->requires = racers;
   finish_lp->synchronous = ATMI_TRUE;
   printf("Now calling and waiting for the finisher ... \n");
   finish(finish_lp,&winner,&loser,finishers);
   printf("The race is over!\n");
}
