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

#define ErrorCheck(status) \
  if (status != ATMI_STATUS_SUCCESS) { \
    printf("Error at [%s:%d]\n", __FILE__, __LINE__); \
    exit(1); \
  }

#define NSECPERSEC 1000000000L
#define NTIMERS 13
long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

int main(int argc, char *argv[]) {
    ErrorCheck(atmi_init(ATMI_DEVTYPE_GPU));

    const char *module = "nullKernel.hsaco";
    atmi_platform_type_t module_type = AMDGCN;
    ErrorCheck(atmi_module_register(&module, &module_type, 1));

    atmi_kernel_t kernel;
    const unsigned int num_args = 0;
    ErrorCheck(atmi_kernel_create(&kernel, num_args, NULL,
          1,
          ATMI_DEVTYPE_GPU, "nullKernel_impl"));

   struct timespec start_time[NTIMERS],end_time[NTIMERS];
   long int kcalls,nanosecs[NTIMERS];
   float kps[NTIMERS];

   kcalls = 2 * 1024;// (128 * 1024);

   ATMI_LPARM(lparm1);
   ATMI_LPARM(lparm2);
   ATMI_LPARM(lparm3);
   ATMI_LPARM(lparm4);

   lparm1->acquire_scope = lparm1->release_scope = ATMI_FENCE_SCOPE_NONE;
   lparm2->acquire_scope = lparm2->release_scope = ATMI_FENCE_SCOPE_NONE;
   lparm3->acquire_scope = lparm3->release_scope = ATMI_FENCE_SCOPE_NONE;
   lparm4->acquire_scope = lparm4->release_scope = ATMI_FENCE_SCOPE_NONE;

   atmi_taskgroup_handle_t stream1_ordered;
   atmi_taskgroup_handle_t stream1_unordered;
   atmi_taskgroup_handle_t stream2_ordered;
   atmi_taskgroup_handle_t stream2_unordered;
   atmi_taskgroup_handle_t stream3_ordered;
   atmi_taskgroup_handle_t stream3_unordered;
   atmi_taskgroup_handle_t stream4_ordered;
   atmi_taskgroup_handle_t stream4_unordered;

   ErrorCheck(atmi_taskgroup_create(&stream1_ordered, true));
   ErrorCheck(atmi_taskgroup_create(&stream2_ordered, true));
   ErrorCheck(atmi_taskgroup_create(&stream3_ordered, true));
   ErrorCheck(atmi_taskgroup_create(&stream4_ordered, true));
   ErrorCheck(atmi_taskgroup_create(&stream1_unordered));
   ErrorCheck(atmi_taskgroup_create(&stream2_unordered));
   ErrorCheck(atmi_taskgroup_create(&stream3_unordered));
   ErrorCheck(atmi_taskgroup_create(&stream4_unordered));

   lparm1->place = ATMI_DEFAULT_PLACE;
   lparm2->place = ATMI_DEFAULT_PLACE;
   lparm3->place = ATMI_DEFAULT_PLACE;
   lparm4->place = ATMI_DEFAULT_PLACE;

   lparm1->group = stream1_ordered;
   lparm2->group = stream2_ordered;
   lparm3->group = stream3_ordered;
   lparm4->group = stream4_ordered;

   lparm1->WORKITEMS=64;
   lparm2->WORKITEMS=64;
   lparm3->WORKITEMS=64;
   lparm4->WORKITEMS=64;

   /* Initialize the Kernel */
   lparm1->synchronous = ATMI_TRUE;
   atmi_task_launch(lparm1, kernel, NULL);

   // synchronous kernels
   lparm1->synchronous = ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
   for(int i=0; i<kcalls; i++) {
    atmi_task_handle_t t = atmi_task_launch(lparm1, kernel, NULL);
    if(t == ATMI_NULL_TASK_HANDLE) {
      fprintf(stderr, "Error launching task\n");
    }
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);

   // asynchronous-ordered kernels
   printf("asynchronous-ordered kernels\n");
   lparm1->synchronous = ATMI_FALSE;
   lparm1->group = stream1_ordered;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
   for(int i=0; i<kcalls; i++) {
    atmi_task_handle_t t = atmi_task_launch(lparm1, kernel, NULL);
    if(t == ATMI_NULL_TASK_HANDLE) {
      fprintf(stderr, "Error launching task\n");
    }
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
   atmi_taskgroup_wait(stream1_ordered);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);

   // asynchronous-unordered kernels
   printf("asynchronous-unordered kernels\n");
   lparm1->synchronous = ATMI_FALSE;
   lparm1->group = stream1_unordered;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[4]);
   for(int i=0; i<kcalls; i++) atmi_task_launch(lparm1, kernel, NULL);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
   atmi_taskgroup_wait(stream1_unordered);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[4]);

   // asynchronous-ordered kernels in 2 taskgroups
   printf("asynchronous-ordered kernels in 2 taskgroups\n");
   lparm1->group = stream1_ordered;
   lparm2->group = stream2_ordered;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[5]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[6]);
   for(int i=0; i<kcalls/2; i++) {
      atmi_task_launch(lparm1, kernel, NULL);
      atmi_task_launch(lparm2, kernel, NULL);
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[5]);
   atmi_taskgroup_wait(stream1_ordered);
   atmi_taskgroup_wait(stream2_ordered);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[6]);

   // asynchronous-unordered kernels in 2 taskgroups
   printf("asynchronous-unordered kernels in 2 taskgroups\n");
   lparm1->group = stream1_unordered;
   lparm2->group = stream2_unordered;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[7]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[8]);
   for(int i=0; i<kcalls/2; i++) {
      atmi_task_launch(lparm1, kernel, NULL);
      atmi_task_launch(lparm2, kernel, NULL);
   }
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[7]);
   atmi_taskgroup_wait(stream1_unordered);
   atmi_taskgroup_wait(stream2_unordered);
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[8]);
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
