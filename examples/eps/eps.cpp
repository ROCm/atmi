#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <libelf.h>
#include <iostream>
#include <sys/param.h> 
#include <time.h>
#include "atmi.h"
#define NSECPERSEC 1000000000L
#define NTIMERS 13
long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

__kernel void nullKernel_impl(__global atmi_task_t *thisTask, long int kcalls) __attribute__((atmi_kernel("nullKernel", "GPU")));

int main(int argc, char *argv[]) {
   struct timespec start_time[NTIMERS],end_time[NTIMERS];
   long int kcalls,nanosecs[NTIMERS];
   float kps[NTIMERS];

   kcalls = 2000;

   ATMI_LPARM_STREAM(lparm1,stream1);
   ATMI_LPARM_STREAM(lparm2,stream2);
   ATMI_LPARM_STREAM(lparm3,stream3);
   ATMI_LPARM_STREAM(lparm4,stream4);

   lparm1->WORKITEMS=64;
   lparm2->WORKITEMS=64;
   lparm3->WORKITEMS=64;
   lparm4->WORKITEMS=64;

   stream1->ordered=ATMI_TRUE;
   lparm1->synchronous=ATMI_TRUE;

   /* Inidialize the Kernel */
   printf("Null Kernel Starts\n");
   fflush(stdout);
   nullKernel(lparm1, kcalls);
#if 0
   printf("Sync Ordered Starts\n");
   fflush(stdout);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
   for(int i=0; i<kcalls; i++) nullKernel(lparm1, kcalls); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);

   printf("Async Ordered Starts\n");
   fflush(stdout);
   lparm1->synchronous=ATMI_FALSE;
   stream1->ordered=ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
   for(int i=0; i<kcalls; i++) nullKernel(lparm1, kcalls); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
   SYNC_STREAM(stream1); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);
#endif
   //printf("Async Unordered Starts\n");
   //fflush(stdout);
   stream1->ordered=ATMI_FALSE;
   lparm1->synchronous=ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[4]);
   for(int i=0; i<kcalls; i++) nullKernel(lparm1, kcalls); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
   SYNC_STREAM(stream1); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[4]);

#if 0
   printf("2 Streams Starts\n");
   fflush(stdout);
   stream1->ordered=ATMI_TRUE;
   stream2->ordered=ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[5]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[6]);
   for(int i=0; i<kcalls/2; i++) {
      nullKernel(lparm1, kcalls); 
      nullKernel(lparm2, kcalls); 
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[5]);
   SYNC_STREAM(stream1); 
   SYNC_STREAM(stream2); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[6]);

   stream1->ordered=ATMI_FALSE;
   stream2->ordered=ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[7]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[8]);
   for(int i=0; i<kcalls/2; i++) {
      nullKernel(lparm1, kcalls); 
      nullKernel(lparm2, kcalls); 
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[7]);
   SYNC_STREAM(stream1); 
   SYNC_STREAM(stream2); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[8]);

   stream1->ordered=ATMI_TRUE;
   stream2->ordered=ATMI_TRUE;
   stream3->ordered=ATMI_TRUE;
   stream4->ordered=ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[9]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[10]);
   for(int i=0; i<kcalls/4; i++) {
      nullKernel(lparm1, kcalls); 
      nullKernel(lparm2, kcalls); 
      nullKernel(lparm3, kcalls); 
      nullKernel(lparm4, kcalls); 
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[9]);
   SYNC_STREAM(stream1); 
   SYNC_STREAM(stream2); 
   SYNC_STREAM(stream3); 
   SYNC_STREAM(stream4); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[10]);

   stream1->ordered=ATMI_FALSE;
   stream2->ordered=ATMI_FALSE;
   stream3->ordered=ATMI_FALSE;
   stream4->ordered=ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[11]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[12]);
   for(int i=0; i<kcalls/4; i++) {
      nullKernel(lparm1, kcalls); 
      nullKernel(lparm2, kcalls); 
      nullKernel(lparm3, kcalls); 
      nullKernel(lparm4, kcalls); 
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[11]);
   SYNC_STREAM(stream1); 
   SYNC_STREAM(stream2); 
   SYNC_STREAM(stream3); 
   SYNC_STREAM(stream4); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[12]);
#endif
   for(int i=0; i<NTIMERS; i++) {
      nanosecs[i] = get_nanosecs(start_time[i],end_time[i]);
      kps[i] = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs[i] ;
   }

   printf("Synchronous Execution  \n");
   printf("Kernel Calls     =  %ld\n",kcalls);
   printf("Secs             =  %6.4f\n",((float)nanosecs[0])/NSECPERSEC);
   printf("Synchronous KPS  =  %6.0f\n\n",kps[0]);
   printf("Asynchronous Ordered Execution in 1 stream (stream->ordered=TRUE)\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[1]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[2])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[2]);
   printf("Asynchronous Unordered Execution in 1 stream (stream->ordered=FALSE)\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[3])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[3]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[4])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[4]);
   printf("Asynchronous Ordered Eecution in 2 streams\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[5])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[5]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[6])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[6]);
   printf("Asynchronous Unordered Execution in 2 streams\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[7])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[7]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[8])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[8]);
   printf("Asynchronous Ordered Execution in 4 streams\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[9])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[9]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[10])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[10]);
   printf("Asynchronous Unordered Execution in 4 streams\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[11])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[11]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[12])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[12]);

}

long int get_nanosecs( struct timespec start_time, struct timespec end_time) {
   long int nanosecs;
   if ((end_time.tv_nsec-start_time.tv_nsec)<0) nanosecs = 
      ((((long int) end_time.tv_sec- (long int) start_time.tv_sec )-1)*NSECPERSEC ) +
      ( NSECPERSEC + (long int) end_time.tv_nsec - (long int) start_time.tv_nsec) ;
   else nanosecs = 
      (((long int) end_time.tv_sec- (long int) start_time.tv_sec )*NSECPERSEC ) +
      ( (long int) end_time.tv_nsec - (long int) start_time.tv_nsec );
   return nanosecs;
}
