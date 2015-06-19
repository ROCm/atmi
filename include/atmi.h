#ifndef __ATMI_H__
/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Asynchronous Task Management Interface ATMI file: atmi.h                   */
/*                                                                            */
/*----------------------------------------------------------------------------*/
#define ATMI_VERSION 0
#define ATMI_RELEASE 1
#define ATMI_PATCH   0  
#define ATMI_VRM ((ATMI_VERSION*65536) + (ATMI_RELEASE*256) + ATMI_PATCH)

/*----------------------------------------------------------------------------*/
/* Enumerated constants and data types                                        */
/*----------------------------------------------------------------------------*/
#define ATMI_ORDERED    1
#define ATMI_UNORDERED  0
#define ATMI_TRUE       1 
#define ATMI_FALSE      0

typedef enum atmi_devtype_s {
    ATMI_DEVTYPE_CPU = 0,
    ATMI_DEVTYPE_GPU = 1,
    ATMI_DEVTYPE_DSP = 2
} atmi_devtype_t;

typedef enum atmi_state_s {
    ATMI_INITIALIZED = 0,
    ATMI_READY       = 1,
    ATMI_DISPATCHED  = 2,
    ATMI_COMPLETED   = 3,
    ATMI_FAILED      = -1
} atmi_state_t;

typedef enum atmi_full_policy_s {
    ATMI_WAIT        = 0,
    ATMI_FAIL        = 1,
    ATMI_DISCARD     = 2
} atmi_full_policy_t;

typedef char boolean; 

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_tprofile_t  ATMI Task Profile Data Structure                          */
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct atmi_tprofile_s {
   unsigned long int dispatch_time;  /*                                       */
   unsigned long int ready_time;     /*                                       */
   unsigned long int start_time;     /*                                       */
   unsigned long int end_time;       /*                                       */
} atmi_tprofile_t;

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_stream_t  ATMI Stream Definition Data Structure                       */
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct atmi_stream_s {
   boolean            ordered;      /*                                        */
   int                maxsize;      /* Number of tasks allowed in stream      */
   atmi_full_policy_t full_policy;  /* What to do if maxsize reached          */
} atmi_stream_t;

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_task_t  ATMI Task Handle Data Structure                               */
/*              All PIF functions return a pointer to atmi_task_t             */ 
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef void* atmi_handle_t;
typedef struct atmi_task_s { 
   atmi_handle_t    handle;
   atmi_state_t     state;    /* Eventually consistent state of task    */
   atmi_tprofile_t* profile;  /* Profile if reqeusted by lparm          */
//   atmi_handle_t    continuation;   /*                                        */
} atmi_task_t;

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_lparm_t  ATMI Launch Parameter Data Structure                         */
/*                                                                            */
/* atmi_lparm_t is the key data structure for ATMI.  It defines the task      */ 
/* launch parameters.  The Platform Interface Function PIF will act on        */
/* information provided by this data structure. The last argument of every    */
/* PIF is an lparm structure.                                                 */
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct atmi_lparm_s { 
   int              ndim;           /* Thread dimensions: 0,1,2, or 3         */
   size_t           gdims[3];       /* # of global threads for each dimension */
   size_t           ldims[3];       /* Thread group size for each dimension   */
   atmi_stream_t*   stream;         /* Group for this task, Default= NULL     */
   boolean          waitable;       /* Create signal for task, default = F    */
   boolean          synchronous;    /* Async or Sync,  default = F (async)    */
   int              acquire_scope;  /* Memory model, default = 2              */
   int              release_scope;  /* Memory model, default = 2              */
   int              num_required;   /* # of required parent tasks, default 0  */
   atmi_task_t**    requires;       /* Array of required parent tasks         */
   int              num_needs_any;  /* # needed parents, only 1 must complete */
   atmi_task_t**    needs_any;      /* Array of needed parent tasks           */
   atmi_devtype_t   devtype;        /* ATMI_DEVTYPE_GPU or ATMI_DEVTYPE_CPU   */
   atmi_tprofile_t* profile;        /* Points to tprofile if metrics desired  */ 
   int              atmi_id;        /* Constant that PIFs can check for       */
   boolean          nested;         /* This task may create more tasks        */
} atmi_lparm_t ;
/*----------------------------------------------------------------------------*/

/* String macros to initialize popular default launch parameters.             */ 
#define ATMI_LPARM_CPU(X) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.ndim=0,.gdims={1},.ldims={1},.stream=NULL,.waitable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.devtype=ATMI_DEVTYPE_CPU,.profile=NULL,.atmi_id=ATMI_VRM,.nested=ATMI_FALSE} ; X = &_ ## X ;

#define ATMI_LPARM_1D(X,Y) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.ndim=1,.gdims={Y},.ldims={64},.stream=NULL,.waitable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.devtype=ATMI_DEVTYPE_GPU,.profile=NULL,.atmi_id=ATMI_VRM,.nested=ATMI_FALSE} ; X = &_ ## X ;
 
#define ATMI_LPARM_2D(X,Y,Z) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.ndim=2,.gdims={Y,Z},.ldims={64,8},.stream=NULL,.waitable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.devtype=ATMI_DEVTYPE_GPU,.profile=NULL,.atmi_id=ATMI_VRM,.nested=ATMI_FALSE} ; X = &_ ## X ;
 
#define ATMI_LPARM_3D(X,Y,Z,V) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.ndim=3,.gdims={Y,Z,V},.ldims={8,8,8},.stream=NULL,.waitable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.devtype=ATMI_DEVTYPE_GPU,.profile=NULL,.atmi_id=ATMI_VRM,.nested=ATMI_FALSE} ; X = &_ ## X ;

#define ATMI_STREAM(NAME) atmi_stream_t * NAME; atmi_stream_t _ ## NAME ={.ordered=ATMI_TRUE} ; NAME = &_ ## NAME ; 

#define ATMI_PROFILE(NAME) NAME = malloc(sizeof(atmi_tprofile_t));

#define ATMI_PROFILE_NEW(NAME) atmi_tprofile_t * NAME ; atmi_tprofile_t _ ## NAME ={.dispatch_time=0,.ready_time=0,.start_time=0,.end_time=0} ; NAME = &_ ## NAME;

/*----------------------------------------------------------------------------*/
/* String macros that look like an API, but actually implement feature by     */
/* calling a null kernel under specific conditions.                           */ 
/*----------------------------------------------------------------------------*/
#if 0
#define SYNC_STREAM(stream) \
{ \
    ATMI_LPARM_CPU(__lparm_sync_kernel); \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->stream = stream; \
    __sync_kernel_cpu(__lparm_sync_kernel); \
}

#define SYNC_TASK(task) \
{ \
    ATMI_LPARM_CPU(__lparm_sync_kernel); \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->num_required = 1; \
    __lparm_sync_kernel->requires = &task; \
    __sync_kernel_cpu(__lparm_sync_kernel); \
}
#endif
/*----------------------------------------------------------------------------*/
/* ATMI Example: HelloWorld                                                   */ 
/*----------------------------------------------------------------------------*/
/* 
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "atmi.h"
#include "hw.h"
int main(int argc, char* argv[]) {
	const char* input = "Gdkkn\x1FGR@\x1FVnqkc";
	size_t strlength = strlen(input);
	char *output = (char*) malloc(strlength + 1);
        ATMI_LPARM_1D(lparm,strlength);
        lparm->synchronous=ATMI_TRUE;
        decode(input,output,lparm);
	output[strlength] = '\0';
	cout << output << endl;
	free(output);
	return 0;
}
__kernel void decode(__global const char* in, __global char* out) {
	out[get_global_id(0)] = in[get_global_id(0)] + 1;
}
*/
#define __ATMI_H__
#endif //__ATMI_H__
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
extern _CPPSTRING_ void atmi_stream_sync(atmi_stream_t *stream);
extern _CPPSTRING_ void atmi_task_wait(atmi_task_t *task);
