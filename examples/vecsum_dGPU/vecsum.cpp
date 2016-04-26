#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include <sys/param.h> 
#include <time.h>
#include "atmi.h"
#include "atmi_runtime.h"

#define NSECPERSEC 1000000000L
#define VECLEN (102500000)
#define SUMS_PER_WKG 8192
#define WKG_SIZE 512
#define INT_TYPE int 
long int get_nanosecs( struct timespec start_time, struct timespec end_time) ;

__kernel void sum8192Kernel_gpu(atmi_task_handle_t thisTask, __global const INT_TYPE * x,  __global INT_TYPE * result)  __attribute((atmi_kernel("sum8192Kernel", "gpu")));
__kernel void sum8192KernelN_gpu(atmi_task_handle_t thisTask, __global const INT_TYPE * x, __global INT_TYPE * result, const int N)  __attribute((atmi_kernel("sum8192KernelN", "gpu")));

extern "C" void sum8192Kernel_cpu(atmi_task_handle_t thisTask, const INT_TYPE * x,  INT_TYPE * result) __attribute__((atmi_kernel("sum8192Kernel", "cpu")));
extern "C" void sum8192KernelN_cpu(atmi_task_handle_t thisTask, const INT_TYPE * x,  INT_TYPE * result, const int N) __attribute__((atmi_kernel("sum8192KernelN", "cpu")));

extern "C" void sum8192Kernel_cpu(atmi_task_handle_t thisTask, const INT_TYPE * x,  INT_TYPE * result) {
    int i = 0;
    result[0] = 0;
    for(i = 0; i < 8192; i++) {
        result[0] += x[i];
    }
}

extern "C" void sum8192KernelN_cpu(atmi_task_handle_t thisTask, const INT_TYPE * x, INT_TYPE * result, const int N) {
    int i = 0;
    result[0] = 0;
    for(i = 0; i < N; i++) {
        result[0] += x[i];
    }
}


int main(int argc, char **argv) {

   struct timespec start_time[6],end_time[6];
   long int nanosecs[6];
   float kps[6];
   int vec1_len = ((VECLEN-1)/SUMS_PER_WKG) + 1;
   int vec2_len = ((vec1_len-1)/SUMS_PER_WKG) + 1; 
   INT_TYPE result;
   INT_TYPE *invec,*outvec1, *outvec2;
   int rc=posix_memalign((void**)&invec,8192,VECLEN*sizeof(INT_TYPE));
   rc = posix_memalign((void**)&outvec1,8192,vec1_len*sizeof(INT_TYPE));
   rc = posix_memalign((void**)&outvec2,8192,vec2_len*sizeof(INT_TYPE));

   /* Initialize */
   for(int i=0; i<VECLEN; i++)  invec[i]=2; 
   
   int kcalls = vec1_len + vec2_len;
   printf("Workgroup size     =  %10d\n",WKG_SIZE);
   printf("Sums per wkgroup   =  %10d\n",SUMS_PER_WKG);
   printf("Vector length      =  %10d\n",VECLEN);
   printf("Pass 1 sums        =  %10d\n",vec1_len);
   printf("Pass 2 sums        =  %10d\n",vec2_len);
   printf("Total kernels      =  %10d\n\n",kcalls);

   printf("CPU Execution\n");
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[0]);
   result = 0;
   for(int i=0; i<VECLEN; i++)  result += invec[i];
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[0]);
   nanosecs[0] = get_nanosecs(start_time[0],end_time[0]);
   printf("  Sum result       =  %10d\n",result);
   printf("  Time on CPU      =  %10.8f secs \n\n",((float)nanosecs[0])/NSECPERSEC);

   printf("Synchronous Execution\n");
   ATMI_LPARM_1D(lparm1,WKG_SIZE);
   lparm1->groupable = ATMI_TRUE;
   lparm1->groupDim[0] = WKG_SIZE;
   lparm1->synchronous = ATMI_TRUE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
   lparm1->kernel_id = K_ID_sum8192Kernel_gpu;
   for(int i=0; i<vec1_len-1; i++) sum8192Kernel(lparm1,&invec[i*SUMS_PER_WKG],&outvec1[i]); 
   lparm1->kernel_id = K_ID_sum8192KernelN_gpu;
   sum8192KernelN(lparm1, &invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1],((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1); 
   //sum8192KernelN(lparm1, ((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1,&invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1],42); 

   lparm1->kernel_id = K_ID_sum8192Kernel_gpu;
   for(int i=0; i<vec2_len-1; i++)  sum8192Kernel(lparm1, &outvec1[i*SUMS_PER_WKG],&outvec2[i]); 
   lparm1->kernel_id = K_ID_sum8192KernelN_gpu;
   sum8192KernelN(lparm1, &outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1],((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1); 
   //sum8192KernelN(lparm1, ((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1], 42); 
   result = 0;
   for(int i=0; i<vec2_len; i++)  result += outvec2[i]; 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
   nanosecs[1] = get_nanosecs(start_time[1],end_time[1]);
   kps[1] = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs[1] ;
   printf("  Result value     =  %10d\n",result);
   printf("  Synchrnous Secs  =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
   printf("  Kernels Per Sec  =  %10.0f  (KPS)\n\n",kps[1]);

   printf("Asynchronous Unordered Execution in 1 stream\n");
   atmi_task_group_t t_group;
   t_group.ordered = ATMI_FALSE;
   lparm1->group = &t_group;

   lparm1->synchronous = ATMI_FALSE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
   lparm1->kernel_id = K_ID_sum8192Kernel_gpu;
   for(int i=0; i<vec1_len-1; i++) sum8192Kernel(lparm1, &invec[i*SUMS_PER_WKG],&outvec1[i]); 
   lparm1->kernel_id = K_ID_sum8192KernelN_gpu;
   sum8192KernelN(lparm1, &invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1], ((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1); 
   //sum8192KernelN(lparm1, ((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1], 42); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);

   lparm1->kernel_id = K_ID_sum8192Kernel_gpu;
   for(int i=0; i<vec2_len-1; i++)  sum8192Kernel(lparm1, &outvec1[i*SUMS_PER_WKG],&outvec2[i]); 
   lparm1->kernel_id = K_ID_sum8192KernelN_gpu;
   sum8192KernelN(lparm1, &outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1],  ((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1); 
   //sum8192KernelN(lparm1, ((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1,&outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1], 42); 
   SYNC_STREAM(0);
   result = 0;
   for(int i=0; i<vec2_len; i++)  result += outvec2[i]; 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[3]);
   nanosecs[2] = get_nanosecs(start_time[2],end_time[2]);
   kps[2] = ((float) vec1_len * (float) NSECPERSEC) / (float) nanosecs[2] ;
   nanosecs[3] = get_nanosecs(start_time[3],end_time[3]);
   kps[3] = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs[3] ;
   printf("  Result value     =  %10d\n",result);
   printf("  Secs to dispatch =  %10.8f\n",((float)nanosecs[2])/NSECPERSEC);
   printf("  KPS dispatched   =  %10.0f \n",kps[2]);
   printf("  Secs to complete =  %10.8f\n",((float)nanosecs[3])/NSECPERSEC);
   printf("  Kernels Per Sec  =  %10.0f  (KPS)\n\n",kps[3]);

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
