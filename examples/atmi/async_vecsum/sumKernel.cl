#define INT_TYPE int
__kernel void sum4096Kernel(__global atmi_task_t *thisTask, __global const INT_TYPE * x,  __global INT_TYPE * result) {
   __local INT_TYPE buffer[512];
   int gid=get_global_id(0);
   buffer[gid] = x[gid] + x[gid+512] + x[gid+1024] + x[gid+1536] +
                 x[gid+2048] + x[gid+2560] + x[gid+3072] + x[gid+3584];
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<256)  buffer[gid] = buffer[gid]+buffer[gid+256]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<128)  buffer[gid] = buffer[gid]+buffer[gid+128]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<64)   buffer[gid] = buffer[gid]+buffer[gid+64];  
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<32)   buffer[gid] = buffer[gid]+buffer[gid+32]; 
   if(gid<16)   buffer[gid] = buffer[gid]+buffer[gid+16];   
   if(gid<8)    buffer[gid] = buffer[gid]+buffer[gid+8]; 
   if(gid<4)    buffer[gid] = buffer[gid]+buffer[gid+4];  
   if(gid<2)    buffer[gid] = buffer[gid]+buffer[gid+2];
   if(gid == 0) result[0] = buffer[0] + buffer[1]; 
}
__kernel void sum4096KernelN(__global atmi_task_t *thisTask, const int N, __global const INT_TYPE * x, __global INT_TYPE * result) {
   __local INT_TYPE buffer[512];
   int gid=get_global_id(0) ;
   buffer[gid] = 0;
    if ( gid      < N ) buffer[gid] = x[gid]; 
    if ( gid+512  < N ) buffer[gid] += x[gid+512]; 
    if ( gid+1024 < N ) buffer[gid] += x[gid+1024]; 
    if ( gid+1536 < N ) buffer[gid] += x[gid+1526]; 
    if ( gid+2048 < N ) buffer[gid] += x[gid+2048]; 
    if ( gid+2560 < N ) buffer[gid] += x[gid+2560]; 
    if ( gid+3072 < N ) buffer[gid] += x[gid+3072]; 
    if ( gid+3584 < N ) buffer[gid] += x[gid+3584]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if (gid < N) {
      if(gid<256) buffer[gid]=buffer[gid]+buffer[gid+256]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<128) buffer[gid]=buffer[gid]+buffer[gid+128]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<64) buffer[gid]=buffer[gid]+buffer[gid+64]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<32) buffer[gid]=buffer[gid]+buffer[gid+32]; 
      if(gid<16) buffer[gid]=buffer[gid]+buffer[gid+16];   
      if(gid<8) buffer[gid]=buffer[gid]+buffer[gid+8]; 
      if(gid<4) buffer[gid]=buffer[gid]+buffer[gid+4];  
      if(gid<2) buffer[gid]=buffer[gid]+buffer[gid+2];
      if(gid == 0) result[0] = buffer[0] +buffer[1];
   }
}

__kernel void sum8192Kernel(__global atmi_task_t *thisTask, __global const INT_TYPE * x,  __global INT_TYPE * result) {
   __local INT_TYPE buffer[512];
   int gid=get_global_id(0);
   buffer[gid] = x[gid] + x[gid+512] + x[gid+1024] + x[gid+1536] +
                 x[gid+2048] + x[gid+2560] + x[gid+3072] + x[gid+3584] +
                 x[gid+4096] + x[gid+4608] + x[gid+5120] + x[gid+5632] +
                 x[gid+6144] + x[gid+6656] + x[gid+7168] + x[gid+7680] ;
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<256)  buffer[gid] = buffer[gid]+buffer[gid+256]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<128)  buffer[gid] = buffer[gid]+buffer[gid+128]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<64)   buffer[gid] = buffer[gid]+buffer[gid+64];  
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<32)   buffer[gid] = buffer[gid]+buffer[gid+32]; 
   if(gid<16)   buffer[gid] = buffer[gid]+buffer[gid+16];   
   if(gid<8)    buffer[gid] = buffer[gid]+buffer[gid+8]; 
   if(gid<4)    buffer[gid] = buffer[gid]+buffer[gid+4];  
   if(gid<2)    buffer[gid] = buffer[gid]+buffer[gid+2];
   if(gid == 0) result[0] = buffer[0] + buffer[1]; 
}
__kernel void sum8192KernelN(__global atmi_task_t *thisTask, const int N, __global const INT_TYPE * x, __global INT_TYPE * result) {
   __local INT_TYPE buffer[512];
   int gid=get_global_id(0) ;
   buffer[gid] = 0;
    if ( gid      < N ) buffer[gid] = x[gid]; 
    if ( gid+512  < N ) buffer[gid] += x[gid+512]; 
    if ( gid+1024 < N ) buffer[gid] += x[gid+1024]; 
    if ( gid+1536 < N ) buffer[gid] += x[gid+1526]; 
    if ( gid+2048 < N ) buffer[gid] += x[gid+2048]; 
    if ( gid+2560 < N ) buffer[gid] += x[gid+2560]; 
    if ( gid+3072 < N ) buffer[gid] += x[gid+3072]; 
    if ( gid+3584 < N ) buffer[gid] += x[gid+3584]; 
    if ( gid+4096 < N ) buffer[gid] += x[gid+4096]; 
    if ( gid+4608 < N ) buffer[gid] += x[gid+4608]; 
    if ( gid+5120 < N ) buffer[gid] += x[gid+5120]; 
    if ( gid+5632 < N ) buffer[gid] += x[gid+5632]; 
    if ( gid+6144 < N ) buffer[gid] += x[gid+6144]; 
    if ( gid+6656 < N ) buffer[gid] += x[gid+6656]; 
    if ( gid+7168 < N ) buffer[gid] += x[gid+7168]; 
    if ( gid+7680 < N ) buffer[gid] += x[gid+7680]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if (gid < N) {
      if(gid<256) buffer[gid]=buffer[gid]+buffer[gid+256]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<128) buffer[gid]=buffer[gid]+buffer[gid+128]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<64) buffer[gid]=buffer[gid]+buffer[gid+64]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<32) buffer[gid]=buffer[gid]+buffer[gid+32]; 
      if(gid<16) buffer[gid]=buffer[gid]+buffer[gid+16];   
      if(gid<8) buffer[gid]=buffer[gid]+buffer[gid+8]; 
      if(gid<4) buffer[gid]=buffer[gid]+buffer[gid+4];  
      if(gid<2) buffer[gid]=buffer[gid]+buffer[gid+2];
      if(gid == 0) result[0] = buffer[0] +buffer[1];
   }
}
__kernel void sum1024Kernel(__global atmi_task_t *thisTask, __global const INT_TYPE * x,  __global INT_TYPE * result) {
   __local INT_TYPE buffer[512];
   int gid=get_global_id(0);
   buffer[gid] = x[gid] + x[gid+512];
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<256)  buffer[gid] = buffer[gid]+buffer[gid+256]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<128)  buffer[gid] = buffer[gid]+buffer[gid+128]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<64)   buffer[gid] = buffer[gid]+buffer[gid+64];  
   barrier(CLK_LOCAL_MEM_FENCE);
   if(gid<32)   buffer[gid] = buffer[gid]+buffer[gid+32]; 
   if(gid<16)   buffer[gid] = buffer[gid]+buffer[gid+16];   
   if(gid<8)    buffer[gid] = buffer[gid]+buffer[gid+8]; 
   if(gid<4)    buffer[gid] = buffer[gid]+buffer[gid+4];  
   if(gid<2)    buffer[gid] = buffer[gid]+buffer[gid+2];
   if(gid == 0) result[0] = buffer[0] + buffer[1]; 
}
__kernel void sum1024KernelN(__global atmi_task_t *thisTask, const int N, __global const INT_TYPE * x, __global INT_TYPE * result) {
   __local INT_TYPE buffer[512];
   int gid=get_global_id(0) ;
   buffer[gid] = 0;
    if ( gid < N  )  buffer[gid] = x[gid]; 
    if ( gid+512 < N ) buffer[gid] += x[gid+512]; 
   barrier(CLK_LOCAL_MEM_FENCE);
   if (gid < N) {
      if(gid<256) buffer[gid]=buffer[gid]+buffer[gid+256]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<128) buffer[gid]=buffer[gid]+buffer[gid+128]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<64) buffer[gid]=buffer[gid]+buffer[gid+64]; 
   barrier(CLK_LOCAL_MEM_FENCE);
      if(gid<32) buffer[gid]=buffer[gid]+buffer[gid+32]; 
      if(gid<16) buffer[gid]=buffer[gid]+buffer[gid+16];   
      if(gid<8) buffer[gid]=buffer[gid]+buffer[gid+8]; 
      if(gid<4) buffer[gid]=buffer[gid]+buffer[gid+4];  
      if(gid<2) buffer[gid]=buffer[gid]+buffer[gid+2];
      if(gid == 0) result[0] = buffer[0] +buffer[1];
   }
}

__kernel void sum64Kernel(__global atmi_task_t *thisTask, __global const INT_TYPE *x,  __global INT_TYPE * result) {
   __local INT_TYPE buffer[64];
   int gid=get_global_id(0);
   buffer[gid] = x[gid] ;
   if(gid<32) buffer[gid]=buffer[gid]+buffer[gid+32]; 
   if(gid<16) buffer[gid]=buffer[gid]+buffer[gid+16];   
   if(gid<8) buffer[gid]=buffer[gid]+buffer[gid+8]; 
   if(gid<4) buffer[gid]=buffer[gid]+buffer[gid+4];  
   if(gid<2) buffer[gid]=buffer[gid]+buffer[gid+2];
   if(gid == 0) result[0] = buffer[0] + buffer[1]; 
}
__kernel void sum64KernelN(__global atmi_task_t *thisTask, const int N, __global const INT_TYPE *x, __global INT_TYPE * result) {
   __local INT_TYPE buffer[64];
   int gid=get_global_id(0);
   buffer[gid] = 0;
   barrier(CLK_LOCAL_MEM_FENCE);
   if (gid < N) {
      buffer[gid] = x[gid];
      if(gid<32) buffer[gid]=buffer[gid]+buffer[gid+32]; 
      if(gid<16) buffer[gid]=buffer[gid]+buffer[gid+16];   
      if(gid<8) buffer[gid]=buffer[gid]+buffer[gid+8]; 
      if(gid<4) buffer[gid]=buffer[gid]+buffer[gid+4];  
      if(gid<2) buffer[gid]=buffer[gid]+buffer[gid+2];
      if(gid == 0) result[0] = buffer[0] +buffer[1];
   }
}
