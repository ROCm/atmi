/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "atmi_runtime.h"
#include <time.h>

enum {
    GPU_IMPL = 42
};    

#define NSECPERSEC 1000000000L
#define NTIMERS 13
long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

int main(int argc, char *argv[]) {
    atmi_status_t err = atmi_init(ATMI_DEVTYPE_ALL);
    if(err != ATMI_STATUS_SUCCESS) return -1;
#ifndef USE_BRIG
    const char *module = "nullKernel.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
#else
    const char *module = "nullKernel.brig";
    atmi_platform_type_t module_type = BRIG;
#endif
    err = atmi_module_register(&module, &module_type, 1);

    atmi_kernel_t kernel;
    const unsigned int num_args = 0;
    atmi_kernel_create_empty(&kernel, num_args, NULL);
    atmi_kernel_add_gpu_impl(kernel, "nullKernel_impl", GPU_IMPL);

   struct timespec start_time[NTIMERS],end_time[NTIMERS];
   long int kcalls,nanosecs[NTIMERS];
   float kps[NTIMERS];

   kcalls = 32 * 1024;// (128 * 1024);

   ATMI_LPARM_STREAM(lparm1,stream1);
   ATMI_LPARM_STREAM(lparm2,stream2);
   ATMI_LPARM_STREAM(lparm3,stream3);
   ATMI_LPARM_STREAM(lparm4,stream4);

   lparm1->kernel_id = GPU_IMPL;
   lparm2->kernel_id = GPU_IMPL;
   lparm3->kernel_id = GPU_IMPL;
   lparm4->kernel_id = GPU_IMPL;
   
   lparm1->WORKITEMS=64;
   lparm2->WORKITEMS=64;
   lparm3->WORKITEMS=64;
   lparm4->WORKITEMS=64;

   lparm1->groupable=ATMI_TRUE;
   lparm2->groupable=ATMI_TRUE;
   lparm3->groupable=ATMI_TRUE;
   lparm4->groupable=ATMI_TRUE;

   /* Initialize the Kernel */
   lparm1->synchronous=ATMI_TRUE;
   atmi_task_launch(lparm1, kernel, NULL);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
   for(int i=0; i<kcalls; i++) atmi_task_launch(lparm1, kernel, NULL);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);
   lparm1->synchronous=ATMI_FALSE;

#if 1
   stream1->ordered=ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
   for(int i=0; i<kcalls; i++) atmi_task_launch(lparm1, kernel, NULL);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
   atmi_task_group_sync(stream1);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);
#endif
#if 1
   stream1->ordered = ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[4]);
   for(int i=0; i<kcalls; i++) atmi_task_launch(lparm1, kernel, NULL);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
   atmi_task_group_sync(stream1);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[4]);
#endif
#if 1
   stream1->ordered=ATMI_TRUE;
   stream2->ordered=ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[5]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[6]);
   for(int i=0; i<kcalls/2; i++) {
      atmi_task_launch(lparm1, kernel, NULL);
      atmi_task_launch(lparm2, kernel, NULL);
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[5]);
   atmi_task_group_sync(stream1);
   atmi_task_group_sync(stream2);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[6]);

   stream1->ordered=ATMI_FALSE;
   stream2->ordered=ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[7]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[8]);
   for(int i=0; i<kcalls/2; i++) {
      atmi_task_launch(lparm1, kernel, NULL);
      atmi_task_launch(lparm2, kernel, NULL);
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[7]);
   atmi_task_group_sync(stream1);
   atmi_task_group_sync(stream2);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[8]);
#endif
#if 0
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
   printf("Asynchronous Ordered Execution in 1 task group\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[1]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[2])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[2]);
   printf("Asynchronous Unordered Execution in 1 task group\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[3])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[3]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[4])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[4]);
   printf("Asynchronous Ordered Execution in 2 task groups\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[5])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[5]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[6])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[6]);
   printf("Asynchronous Unordered Execution in 2 task groups\n");
   printf("Secs to dispatch =  %10.8f\n",((float)nanosecs[7])/NSECPERSEC);
   printf("KPS dispatched   =  %6.0f\n",kps[7]);
   printf("Secs to complete =  %10.8f\n",((float)nanosecs[8])/NSECPERSEC);
   printf("KPS completed    =  %6.0f\n\n",kps[8]);
#if 0
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
   atmi_finalize();
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
