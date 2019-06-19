/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <libelf.h>
#include <iostream>
#include <sys/param.h> 
#include <time.h>
#include "atmi.h"
#include "atmi_runtime.h"
#define NSECPERSEC 1000000000L
#define NTIMERS 13
long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

__kernel void nullKernel_impl(long int kcalls) __attribute__((atmi_kernel("nullKernel", "GPU")));

int main(int argc, char *argv[]) {
   struct timespec start_time[NTIMERS],end_time[NTIMERS];
   long int kcalls,nanosecs[NTIMERS];
   float kps[NTIMERS];

   kcalls = (128*1024);

   ATMI_LPARM(lparm1);
   ATMI_LPARM(lparm2);
   ATMI_LPARM(lparm3);
   ATMI_LPARM(lparm4);

   atmi_taskgroup_handle_t stream1_ordered;
   atmi_taskgroup_handle_t stream1_unordered;
   atmi_taskgroup_handle_t stream2_ordered;
   atmi_taskgroup_handle_t stream2_unordered;
   atmi_taskgroup_handle_t stream3_ordered;
   atmi_taskgroup_handle_t stream3_unordered;
   atmi_taskgroup_handle_t stream4_ordered;
   atmi_taskgroup_handle_t stream4_unordered;

   err = atmi_taskgroup_create(&stream1_ordered);
   err = atmi_taskgroup_create(&stream2_ordered);
   err = atmi_taskgroup_create(&stream3_ordered);
   err = atmi_taskgroup_create(&stream4_ordered);
   err = atmi_taskgroup_create(&stream1_unordered);
   err = atmi_taskgroup_create(&stream2_unordered);
   err = atmi_taskgroup_create(&stream3_unordered);
   err = atmi_taskgroup_create(&stream4_unordered);

   lparm1->group = stream1_ordered;
   lparm2->group = stream2_ordered;
   lparm3->group = stream3_ordered;
   lparm4->group = stream4_ordered;

   lparm1->kernel_id = K_ID_nullKernel_impl;
   lparm2->kernel_id = K_ID_nullKernel_impl;
   lparm3->kernel_id = K_ID_nullKernel_impl;
   lparm4->kernel_id = K_ID_nullKernel_impl;
   
   lparm1->WORKITEMS=64;
   lparm2->WORKITEMS=64;
   lparm3->WORKITEMS=64;
   lparm4->WORKITEMS=64;

   /* Initialize the Kernel */
   lparm1->groupable=ATMI_FALSE;
   lparm1->synchronous=ATMI_TRUE;
   nullKernel(lparm1, kcalls);
#if 1
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
//   for(int i=0; i<kcalls; i++) nullKernel(lparm1, kcalls); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);
   //atmi_taskgroup_wait(stream1);

   lparm1->synchronous=ATMI_FALSE;
   lparm1->group = stream1_ordered;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
   for(int i=0; i<kcalls; i++) nullKernel(lparm1, kcalls); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
   //SYNC_STREAM(stream1); 
   atmi_taskgroup_wait(stream1_ordered);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);
#endif
#if 1
   lparm1->group = stream1_unordered;
   lparm1->synchronous = ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[4]);
   for(int i=0; i<kcalls; i++) nullKernel(lparm1, kcalls); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
   //SYNC_STREAM(stream1); 
   atmi_taskgroup_wait(stream1_unordered);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[4]);
#endif
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
   printf("Asynchronous Tasks Ordered Stream in 1 stream\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[1]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[2])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[2]);
   printf("Asynchronous Tasks Unordered Stream in 1 stream\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[3])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[3]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[4])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[4]);
#if 0
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
#endif 
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
