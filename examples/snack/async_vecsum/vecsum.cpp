#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <libelf.h>
#include <iostream>
#include <sys/param.h> 
#include <time.h>
#include "sumKernel.h"
#define NSECPERSEC 1000000000L
#define VECLEN 102500000
#define SUMS_PER_WKG 8192
#define WKG_SIZE 512
#define INT_TYPE int 

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
   SNK_INIT_LPARM(lparm1,WKG_SIZE);
   lparm1->ldims[0] = WKG_SIZE;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[1]);
   for(int i=0; i<vec1_len-1; i++) sum8192Kernel(&invec[i*SUMS_PER_WKG],&outvec1[i],lparm1); 
   sum8192KernelN( ((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1],lparm1); 

   for(int i=0; i<vec2_len-1; i++)  sum8192Kernel(&outvec1[i*SUMS_PER_WKG],&outvec2[i],lparm1); 
   sum8192KernelN( ((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1],lparm1); 
   result = 0;
   for(int i=0; i<vec2_len; i++)  result += outvec2[i]; 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[1]);
   nanosecs[1] = get_nanosecs(start_time[1],end_time[1]);
   kps[1] = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs[1] ;
   printf("  Result value     =  %10d\n",result);
   printf("  Synchrnous Secs  =  %10.8f\n",((float)nanosecs[1])/NSECPERSEC);
   printf("  Kernels Per Sec  =  %10.0f  (KPS)\n\n",kps[1]);

   printf("Asynchronous Unordered Execution in 1 stream\n");
   lparm1->stream = 0;
   lparm1->barrier = SNK_UNORDERED;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[2]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[3]);
   for(int i=0; i<vec1_len-1; i++) sum8192Kernel(&invec[i*SUMS_PER_WKG],&outvec1[i],lparm1); 
   sum8192KernelN( ((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1],lparm1); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[2]);
   stream_sync(0);  /* Wait for all the kernels to complete before next pass */
   for(int i=0; i<vec2_len-1; i++)  sum8192Kernel(&outvec1[i*SUMS_PER_WKG],&outvec2[i],lparm1); 
   sum8192KernelN( ((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1],lparm1); 
   stream_sync(0);  /* Wait for all the kernels to complete before summing remaining terms */
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


   printf("Asynchronous Unordered Execution in 2 streams \n");
   lparm1->stream=1;
   SNK_INIT_LPARM(lparm2,WKG_SIZE);
   lparm2->ldims[0] = WKG_SIZE;
   lparm2->stream=2;
   lparm2->barrier=SNK_UNORDERED;
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[4]);
   clock_gettime(CLOCK_MONOTONIC_RAW,&start_time[5]);
   for(int i=0; i<vec1_len-2; i+=2 ) {
      sum8192Kernel(&invec[i*SUMS_PER_WKG],&outvec1[i],lparm1); 
      sum8192Kernel(&invec[(i+1)*SUMS_PER_WKG],&outvec1[i+1],lparm2); 
   }
   if (( vec1_len % 2 ) == 0 )  
      sum8192Kernel(&invec[(vec1_len-2)*SUMS_PER_WKG],&outvec1[vec1_len-2],lparm1); 
   sum8192KernelN( ((VECLEN+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &invec[(vec1_len-1)*SUMS_PER_WKG],&outvec1[vec1_len-1],lparm2); 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[4]);
   stream_sync(1);  /* Wait for all the kernels to complete before next pass */
   stream_sync(2); 
   for(int i=0; i<vec2_len-2; i+=2 )  {
      sum8192Kernel(&outvec1[i*SUMS_PER_WKG],&outvec2[i],lparm1); 
      sum8192Kernel(&outvec1[(i+1)*SUMS_PER_WKG],&outvec2[i+1],lparm2); 
   }
   if (( vec2_len % 2 ) == 0 )  
      sum8192Kernel(&outvec1[(vec2_len-2)*SUMS_PER_WKG],&outvec2[vec2_len-2],lparm1);  
   sum8192KernelN( ((vec1_len+SUMS_PER_WKG-1) % SUMS_PER_WKG)+1, &outvec1[(vec2_len-1)*SUMS_PER_WKG],&outvec2[vec2_len-1],lparm2); 
   stream_sync(1); 
   stream_sync(2); 
   result = 0;
   for(int i=0; i<vec2_len; i++)  result += outvec2[i]; 
   clock_gettime(CLOCK_MONOTONIC_RAW,&end_time[5]);
   nanosecs[4] = get_nanosecs(start_time[4],end_time[4]);
   kps[4] = ((float) vec1_len * (float) NSECPERSEC) / (float) nanosecs[4] ;
   nanosecs[5] = get_nanosecs(start_time[5],end_time[5]);
   kps[5] = ((float) kcalls * (float) NSECPERSEC) / (float) nanosecs[5] ;
   printf("  Result value     =  %10d\n",result);
   printf("  Secs to dispatch =  %10.8f\n",((float)nanosecs[4])/NSECPERSEC);
   printf("  KPS dispatched   =  %10.0f \n",kps[4]);
   printf("  Secs to complete =  %10.8f\n",((float)nanosecs[5])/NSECPERSEC);
   printf("  Kernels per sec  =  %10.0f  (KPS) \n\n",kps[5]);

}

