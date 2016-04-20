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
    ATMI_DEVTYPE_CPU = (1 << 0),
    ATMI_DEVTYPE_GPU = (1 << 1),
    ATMI_DEVTYPE_DSP = (1 << 2)
} atmi_devtype_t;

typedef enum atmi_memtype_s {
    ATMI_MEMTYPE_KERNARG        = (1 << 0),
    ATMI_MEMTYPE_FINE_GRAINED   = (1 << 1),
    ATMI_MEMTYPE_COARSE_GRAINED = (1 << 2) 
} atmi_memtype_t;

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

#define ATMI_MAX_NODES  1024
#define ATMI_MAX_CUS    64
typedef struct atmi_place_s {
    unsigned int node_id;           /* node_id = 0 for local computations     */
    unsigned long cpu_set;          /* CPU (core) set.                        */
    unsigned long gpu_set;          /* GPU (CU) set.                          */
} atmi_place_t;

#define ATMI_PLACE_ANY(node) {.node_id=node, .cpu_set=0xFFFFFFFFFFFFFFFF, .gpu_set=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_ANY_CPU(node) {.node_id=node, .cpu_set=0xFFFFFFFFFFFFFFFF, .gpu_set=0x0} 
#define ATMI_PLACE_ANY_GPU(node) {.node_id=node, .cpu_set=0x0, .gpu_set=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_CPU(node, cpu_id) {.node_id=node, .cpu_set=(1 << cpu_id), .gpu_set=0x0} 
#define ATMI_PLACE_GPU(node, gpu_id) {.node_id=node, .cpu_set=0x0, .gpu_set=(1 << gpu_id)} 
#define ATMI_PLACE_GPU_MASK(node, gpu_mask) {.node_id=node, .cpu_set=0x0, .gpu_set=(0x0|gpu_mask)} 
#define ATMI_PLACE_CPU_MASK(node, cpu_mask) {.node_id=node, .cpu_set=(0x0|cpu_mask), .gpu_set=0x0} 

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_task_group_t  ATMI Task Group Data Structure                          */
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct atmi_task_group_s {
   int                id;           /* Unique task group identifier           */
   boolean            ordered;      /*                                        */
   atmi_place_t       place;        /* CUs to execute tasks; default: any     */
   int                maxsize;      /* Number of tasks allowed in group       */
   atmi_full_policy_t full_policy;  /* What to do if maxsize reached          */
} atmi_task_group_t;

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_context_t  ATMI Context Data Structure for system information         */
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef struct atmi_context_s {
   int                atmi_id;        /* ATMI version information             */
} atmi_context_t;
extern atmi_context_t* atmi_context;
// Why are we exposing the object AND the pointer to the programmer?
// extern atmi_context_t  atmi_context_data;;

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* atmi_task_t  ATMI Task Handle Data Structure                               */
/*              All PIF functions return a pointer to atmi_task_t             */ 
/*                                                                            */
/*----------------------------------------------------------------------------*/
typedef void* atmi_handle_t;
typedef struct atmi_task_s atmi_task_t;
struct atmi_task_s { 
   atmi_handle_t    handle;
   atmi_state_t     state;    /* Eventually consistent state of task    */
   atmi_tprofile_t  profile;  /* Profile if reqeusted by lparm          */
   atmi_task_t *continuation; /*                                        */
};

typedef struct atmi_task_info_s {
   atmi_state_t     state;    /* Eventually consistent state of task    */
   atmi_tprofile_t  profile;  /* Profile if reqeusted by lparm          */
} atmi_task_info_t;

#if 0
typedef struct atmi_task_handle_s {
    union {
        struct {
            unsigned node : 16;
            unsigned hi : 16;
            unsigned lo : 32;
        };
        unsigned long int all;
    };
} atmi_task_handle_t;
//#define ATMI_TASK_HANDLE(low) (atmi_task_handle_t){.node=0,.hi=0,.lo=low}
#else
typedef unsigned long int atmi_task_handle_t;
#define ATMI_TASK_HANDLE(low) (low)
#endif
extern atmi_task_handle_t NULL_TASK;
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
#if 0
    union {
        struct {
            unsigned long workitems;
            unsigned long workitems2D;
            unsigned long workitems3D;
        };
        unsigned long gridDim[3];
    };
#else
    unsigned long    gridDim[3];     /* # of global threads for each dimension */
#endif
    unsigned long    groupDim[3];    /* Thread group size for each dimension   */
    atmi_task_group_t*   group;      /* Group for this task, Default= NULL     */
    boolean          groupable;      /* Create signal for task, default = F    */
    boolean          synchronous;    /* Async or Sync,  default = F (async)    */
    int              acquire_scope;  /* Memory model, default = 2              */
    int              release_scope;  /* Memory model, default = 2              */
    int              num_required;   /* # of required parent tasks, default 0  */
    atmi_task_handle_t*    requires;       /* Array of required parent tasks         */
    int              num_needs_any;  /* # needed parents, only 1 must complete */
    atmi_task_handle_t*    needs_any;      /* Array of needed parent tasks           */
    boolean          profilable;     /* Points to tprofile if metrics desired  */ 
    int              atmi_id;        /* Constant that PIFs can check for       */
    int              kernel_id;
    atmi_place_t     place;
    atmi_task_t*     task_info;
    atmi_task_handle_t     continuation_task;
} atmi_lparm_t ;
/*----------------------------------------------------------------------------*/

typedef struct atmi_task_list_s {
    atmi_task_t *task;
    struct atmi_task_list_s *next;
} atmi_task_list_t;

typedef struct atmi_data_s {
    void *ptr;
    unsigned long long int size;
    atmi_place_t place;
} atmi_data_t;

#define WORKITEMS gridDim[0] 
#define WORKITEMS2D gridDim[1] 
#define WORKITEMS3D gridDim[2] 

/* String macros to initialize popular default launch parameters.             */ 
#define ATMI_LPARM(X) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={1,1,1},.groupDim={64,1,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=0,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_LPARM_STREAM(X,Y) atmi_task_group_t * Y; atmi_task_group_t _ ## Y ={.id=0,.ordered=ATMI_TRUE} ; Y = &_ ## Y ; atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={1,1,1},.groupDim={64,1,1},.group=Y,.groupable=ATMI_TRUE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=0,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_LPARM_1D(X,Y) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y},.groupDim={64},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=0,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;
 
#define ATMI_LPARM_2D(X,Y,Z) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,Z},.groupDim={64,8},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=0,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;
 
#define ATMI_LPARM_3D(X,Y,Z,V) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,Z,V},.groupDim={8,8,8},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=0,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_STREAM(NAME) atmi_task_group_t * NAME; atmi_task_group_t _ ## NAME ={.ordered=ATMI_TRUE} ; NAME = &_ ## NAME ; 

#define ATMI_PROFILE(NAME) NAME = malloc(sizeof(atmi_tprofile_t));

#define ATMI_PROFILE_NEW(NAME) atmi_tprofile_t * NAME ; atmi_tprofile_t _ ## NAME ={.dispatch_time=0,.ready_time=0,.start_time=0,.end_time=0} ; NAME = &_ ## NAME;

#define ATMI_PUSH_BACK_REQUIRES(REQS, TASK) { \
    atmi_task_list_t *list = (atmi_task_list_t *)malloc(sizeof(atmi_task_list_t));\
    list->task = TASK; \
    list->next = REQS; \
    REQS = list; \
}

#define ATMI_FREE_REQUIRES(REQS) {\
    atmi_task_list_t *cur = REQS; \
    while(cur != NULL) {\
        REQS = cur->next;\
        free(cur); \
        cur = REQS;\
    }\
}

/*----------------------------------------------------------------------------*/
/* String macros that look like an API, but actually implement feature by     */
/* calling a null kernel under specific conditions.                           */ 
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
extern _CPPSTRING_ void atmi_task_group_sync(atmi_task_group_t *group);
extern _CPPSTRING_ atmi_task_handle_t __sync_kernel_pif(atmi_lparm_t *lparm);

#if 1
#define SYNC_STREAM(s) \
{ \
    ATMI_LPARM(__lparm_sync_kernel); \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->group = s; \
    __sync_kernel_pif(__lparm_sync_kernel); \
}

#define SYNC_TASK(t) \
{ \
    ATMI_LPARM(__lparm_sync_kernel); \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->num_required = 1; \
    __lparm_sync_kernel->requires = &t; \
    __sync_kernel_pif(__lparm_sync_kernel); \
}
#endif

    //atmi_task_t temp; \
    //temp.continuation = NULL; \
    //__lparm_sync_kernel->task_info = &temp; \
#define ATMI_myTask __global atmi_task_t*thisTask

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
	unsigned long strlength = strlen(input);
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
