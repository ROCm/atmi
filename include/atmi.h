/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
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

/** \defgroup enumerations Enumerated Types
 * @{
 */
/**
 * @brief Status codes.
 */
typedef enum atmi_status_t {
    /**
     * The function has been executed successfully.
     */
    ATMI_STATUS_SUCCESS=0,
    /**
     * A undocumented error has occurred.
     */
    ATMI_STATUS_UNKNOWN=1,
    /**
     * A generic error has occurred.
     */
    ATMI_STATUS_ERROR=2
} atmi_status_t;

/**
 * @brief Platform Types.
 */
typedef enum {
    /**
     * Target Platform is BRIG (deprecated)
     */
    BRIG = 0,
    /**
     * Target Platform is AMD GCN (default) 
     */
    AMDGCN, 
    /* -- support in the future? -- 
    HSAIL,
    CL,
    x86, 
    PTX
    */
} atmi_platform_type_t;

/**
 * @brief Device Types.
 */
typedef enum atmi_devtype_s {
    ATMI_DEVTYPE_CPU  = 0x0001,  /**< CPU */ 
    ATMI_DEVTYPE_iGPU = 0x0010,  /**< Integrated GPU */
    ATMI_DEVTYPE_dGPU = 0x0100,  /**< Discrete GPU */
    ATMI_DEVTYPE_GPU  = ATMI_DEVTYPE_iGPU | ATMI_DEVTYPE_dGPU, /**< Any GPU */
    ATMI_DEVTYPE_DSP  = 0x1000,  /**< Digitial Signal Processor */
    ATMI_DEVTYPE_ALL  = 0x1111   /**< Union of all device types */
} atmi_devtype_t;

/**
 * @brief Memory Access Type.
 */
typedef enum atmi_memtype_s {
    ATMI_MEMTYPE_FINE_GRAINED   = 0, /**< Fine grained memory type */
    ATMI_MEMTYPE_COARSE_GRAINED = 1, /**< Coarse grained memory type */
    ATMI_MEMTYPE_ANY                 /**< Any memory type */
    // ATMI should not be concerned about kernarg region, which should 
    // be handled by HSA and not by the end application user
} atmi_memtype_t;

/**
 * @brief Task States.
 */
typedef enum atmi_state_s {
    ATMI_UNINITIALIZED = -1, /**< Uninitialized state */
    ATMI_INITIALIZED = 0, /**< Initialized state */
    ATMI_READY       = 1, /**< Ready state */
    ATMI_DISPATCHED  = 2, /**< Dispatched state */
    ATMI_EXECUTED    = 3, /**< Executed state */
    ATMI_COMPLETED   = 4, /**< Completed state */
    ATMI_FAILED      = 9999 /**< Failed state */
} atmi_state_t;

/**
 * @brief Scheduler Types.
 */
typedef enum atmi_scheduler_s {
    ATMI_SCHED_NONE = 0, /**< No scheduler, all tasks go to the same queue */
    ATMI_SCHED_RR        /**< Round-robin scheduler */
} atmi_scheduler_t;

/**
 * @brief ATMI data arg types.
 */
typedef enum atmi_arg_type_s {
    ATMI_IN, /**< Input argument */
    ATMI_OUT,/**< Output argument */
    ATMI_IN_OUT /** In/out argument */
} atmi_arg_type_t;    

#if 0
typedef enum atmi_data_type_s {
    ATMI_CHAR,
    ATMI_UNSIGNED_CHAR,
    ATMI_INT,
    ATMI_UNSIGNED_INT,
    ATMI_LONG,
    ATMI_UNSIGNED_LONG,
    ATMI_LONG_LONG,
    ATMI_UNSIGNED_LONG_LONG,
    ATMI_FLOAT,
    ATMI_DOUBLE,
    ATMI_SIZE,
    ATMI_PTR = (1 << 31)
} atmi_data_type_t;

// More investigation needed to include this enum
typedef enum atmi_full_policy_s {
    ATMI_WAIT        = 0,
    ATMI_FAIL        = 1,
    ATMI_DISCARD     = 2
} atmi_full_policy_t;
#endif 
/** @} */
typedef char boolean; 
//#define ATMI_MAX_NODES  1024
//#define ATMI_MAX_CUS    64

/** \defgroup common Common ATMI Structures
 *  @{
 */
/**                                                                            
 * @brief ATMI Task Profile Data Structure                          
 */
typedef struct atmi_tprofile_s {
   unsigned long int dispatch_time;  /**< Timestamp of task dispatch.         */
   unsigned long int ready_time;     /**< Timestamp when the task's dependencies 
                                          were all met and ready to be 
                                          dispatched.                         */
   unsigned long int start_time;     /**< Timstamp when the task started 
                                          execution.                          */
   unsigned long int end_time;       /**< TImestamp when the task completed
                                          execution.                          */
} atmi_tprofile_t;

/**                                                                            
 * @brief ATMI Compute Place                          
 */
typedef struct atmi_place_s {
    unsigned int node_id;           /**< node_id = 0 for local computations     */
    atmi_devtype_t type;            /**< CPU, GPU or DSP                        */
    int device_id;                  /**< Devices ordered by runtime; -1 for any */
    unsigned long cu_mask;          /**< Compute Unit Mask (advanced feature)   */
} atmi_place_t;

/**                                                                            
 * @brief ATMI Memory Place                          
 */
typedef struct atmi_mem_place_s {
    unsigned int node_id;           /**< node_id = 0 for local computations     */
    atmi_devtype_t dev_type;        /**< CPU, GPU or DSP                        */
    int dev_id;                     /**< Devices ordered by runtime; -1 for any */
  //atmi_memtype_t mem_type;        /**< Fine grained or Coarse grained         */
    int mem_id;                     /**< Memory spaces; -1 for any              */
} atmi_mem_place_t;

/**
 * @brief ATMI Memory Structure
 */
typedef struct atmi_memory_s {
    unsigned long int capacity;
    atmi_memtype_t  type;
} atmi_memory_t;

/**                                                                            
 * @brief ATMI Device Structure
 */
typedef struct atmi_device_s {
    atmi_devtype_t type;            /**< Device type */
    unsigned int core_count;        /**< Number of compute cores */
    unsigned int memory_count;      /**< Number of memory regions that are 
                                         accessible from this device. */
    atmi_memory_t *memories;        /**< The memory regions that are 
                                         accessible from this device. */
} atmi_device_t;

/**                                                                            
 * @brief ATMI Machine Structure
 */
typedef struct atmi_machine_s {
    unsigned int device_count_by_type[ATMI_DEVTYPE_ALL];   /**< The number of devices categorized 
                                                                by the device type */
    atmi_device_t *devices_by_type[ATMI_DEVTYPE_ALL];      /**< The device structures categorized 
                                                                by the device type */
} atmi_machine_t;

/**
 * @brief ATMI Task Group Data Structure
 */
typedef struct atmi_task_group_s {
   int                id;           /**< Unique task group identifier           */
   boolean            ordered;      /**<                                        */
   atmi_place_t       place;        /**< CUs to execute tasks; default: any     */
   int                maxsize;      /**< Number of tasks allowed in group       */
   //atmi_full_policy_t full_policy;/**< What to do if maxsize reached          */
} atmi_task_group_t;

/**
 * @brief ATMI Task info structure
 */
typedef void* atmi_handle_t;
typedef struct atmi_task_s { 
   atmi_handle_t    handle;   /**< Temp storage location for current task handle for DP*/
   atmi_state_t     state;    /**< Previously consistent state of task    */
   atmi_tprofile_t  profile;  /**< Previously consistent profile information */
   struct atmi_task_s *continuation; /**< The continuation task of this current task */
} atmi_task_t;
#if 0
typedef struct atmi_task_info_s {
   atmi_state_t     state;    /* Eventually consistent state of task    */
   atmi_tprofile_t  profile;  /* Profile if reqeusted by lparm          */
} atmi_task_info_t;
#endif 
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
/**
 * @brief The ATMI task handle.
 */
typedef unsigned long int atmi_task_handle_t;
#endif
/**
 * @brief The special NULL task handle. 
 */
extern atmi_task_handle_t ATMI_NULL_TASK_HANDLE;

/**
 * @brief The ATMI Launch Parameter Data Structure
 */
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
    unsigned long    gridDim[3];     /**< # of global threads for each dimension */
#endif
    unsigned long    groupDim[3];    /**< Thread group size for each dimension   */
    atmi_task_group_t*   group;      /**< Group for this task, Default= NULL     */
    boolean          groupable;      /**< Create signal for task, default = F    */
    boolean          synchronous;    /**< Async or Sync,  default = F (async)    */
    int              acquire_scope;  /**< Memory model, default = 2              */
    int              release_scope;  /**< Memory model, default = 2              */
    int              num_required;   /**< # of required parent tasks, default 0  */
    atmi_task_handle_t*    requires; /**< Array of required parent tasks         */
    int              num_required_groups;  /**< # of required parent task groups       */
    atmi_task_group_t**    required_groups;/**< Array of required parent task groups   */
    boolean          profilable;     /**< Points to tprofile if metrics desired  */ 
    int              atmi_id;        /**< Constant that PIFs can check for       */
    int              kernel_id;      /**< Kernel ID if more than one kernel per task */
    atmi_place_t     place;          /**< Compute location to launch this task. */
    atmi_task_t*     task_info;      /**< Optional user-created structure to store 
                                          executed task's information */
    atmi_task_handle_t     continuation_task; /**< The continuation task of 
                                                   this current task */
} atmi_lparm_t ;

/**
 * @brief The ATMI Data Copy Parameter Data Structure
 */
typedef struct atmi_cparm_s {
    atmi_task_group_t*   group;      /**< Group for this task, Default= NULL     */
    boolean          groupable;      /**< Create signal for task, default = F    */
    boolean          profilable;     /**< Points to tprofile if metrics desired  */ 
    boolean          synchronous;    /**< Async or Sync,  default = F (async)    */
    int                 num_required;/**< # of required parent tasks, default 0  */
    atmi_task_handle_t* requires;    /**< Array of required parent tasks         */
    int              num_required_groups;  /**< # of required parent task groups       */
    atmi_task_group_t**    required_groups;/**< Array of required parent task groups   */
    atmi_task_t*     task_info;      /**< Optional user-created structure to store
                                          executed task's information */
} atmi_cparm_t;

/**
 * @brief High-level data abstraction
 */
typedef struct atmi_data_s {
    void *ptr;                  /**< The data pointer */
    //atmi_data_type_t type;
    unsigned int size;          /**< Data size */
    atmi_mem_place_t place;     /**< The memory placement of data */
    // TODO: what other information can be part of data?
} atmi_data_t;
/** @} */

#define ATMI_TASK_HANDLE(low) (low)

#define ATMI_PLACE_ANY(node) {.node_id=node, .type=ATMI_DEVTYPE_ALL, .device_id=-1, .cu_mask=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_ANY_CPU(node) {.node_id=node, .type=ATMI_DEVTYPE_CPU, .device_id=-1, .cu_mask=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_ANY_GPU(node) {.node_id=node, .type=ATMI_DEVTYPE_GPU, .device_id=-1, .cu_mask=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_CPU(node, cpu_id) {.node_id=node, .type=ATMI_DEVTYPE_CPU, .device_id=cpu_id, .cu_mask=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_GPU(node, gpu_id) {.node_id=node, .type=ATMI_DEVTYPE_GPU, .device_id=gpu_id, .cu_mask=0xFFFFFFFFFFFFFFFF} 
#define ATMI_PLACE_CPU_MASK(node, cpu_id, cpu_mask) {.node_id=node, .type=ATMI_DEVTYPE_CPU, device_id=cpu_id, .cu_mask=(0x0|cpu_mask)} 
#define ATMI_PLACE_GPU_MASK(node, gpu_id, gpu_mask) {.node_id=node, .type=ATMI_DEVTYPE_GPU, device_id=gpu_id, .cu_mask=(0x0|gpu_mask)} 
#define ATMI_PLACE(node, dev_type, dev_id, mask) {.node_id=node, .type=dev_type, .device_id=dev_id, .cu_mask=mask} 


#define ATMI_MEM_PLACE_ANY(node) {.node_id=node, .dev_type=ATMI_DEVTYPE_ALL, .dev_id=-1, .mem_id=-1} 
#define ATMI_MEM_PLACE_ANY_CPU(node) {.node_id=node, .dev_type=ATMI_DEVTYPE_CPU, .dev_id=-1, .mem_id=-1} 
#define ATMI_MEM_PLACE_ANY_GPU(node) {.node_id=node, .dev_type=ATMI_DEVTYPE_GPU, .dev_id=-1, .mem_id=-1} 
#define ATMI_MEM_PLACE_CPU(node, cpu_id) {.node_id=node, .dev_type=ATMI_DEVTYPE_CPU, .dev_id=cpu_id, .mem_id=-1} 
#define ATMI_MEM_PLACE_GPU(node, gpu_id) {.node_id=node, .dev_type=ATMI_DEVTYPE_GPU, .dev_id=gpu_id, .mem_id=-1} 
#define ATMI_MEM_PLACE_CPU_MEM(node, cpu_id, cpu_mem_id) {.node_id=node, .dev_type=ATMI_DEVTYPE_CPU, .dev_id=cpu_id, .mem_id=cpu_mem_id} 
#define ATMI_MEM_PLACE_GPU_MEM(node, gpu_id, gpu_mem_id) {.node_id=node, .dev_type=ATMI_DEVTYPE_GPU, .dev_id=gpu_id, .mem_id=gpu_mem_id} 
#define ATMI_MEM_PLACE(d_type, d_id, m_id) {.node_id=0, .dev_type=d_type, .dev_id=d_id, .mem_id=m_id} 
#define ATMI_MEM_PLACE_NODE(node, d_type, d_id, m_id) {.node_id=node, .dev_type=d_type, .dev_id=d_id, .mem_id=m_id} 



#define ATMI_DATA(X, PTR, COUNT, PLACE) atmi_data_t X; X.ptr=PTR; X.size=COUNT; X.place=PLACE;

#define WORKITEMS gridDim[0] 
#define WORKITEMS2D gridDim[1] 
#define WORKITEMS3D gridDim[2] 

/* String macros to initialize popular default launch parameters.             */ 
#define ATMI_CPARM(X) atmi_cparm_t * X ; atmi_cparm_t  _ ## X ={.group=NULL,.groupable=ATMI_FALSE,.profilable=ATMI_FALSE,.synchronous=ATMI_FALSE,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.task_info=NULL} ; X = &_ ## X ;

#ifndef __OPENCL_C_VERSION__ 
#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __LINE__)
#define ATMI_PARM_SET_NAMED_DEPENDENCIES(X, HANDLES, ...) \
    atmi_task_handle_t (HANDLES)[] = { __VA_ARGS__ }; \
    (X)->num_required = sizeof(HANDLES)/sizeof(HANDLES[0]); \
    (X)->requires = HANDLES; 

#define ATMI_PARM_SET_DEPENDENCIES(X, ...) ATMI_PARM_SET_NAMED_DEPENDENCIES(X, MAKE_UNIQUE(handles), __VA_ARGS__)
#endif

#define ATMI_LPARM(X) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={1,1,1},.groupDim={1,1,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_LPARM_STREAM(X,Y) atmi_task_group_t * Y; atmi_task_group_t _ ## Y ={.id=0,.ordered=ATMI_TRUE} ; Y = &_ ## Y ; atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={1,1,1},.groupDim={64,1,1},.group=Y,.groupable=ATMI_TRUE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_LPARM_CPU(X,CPU) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={1,1,1},.groupDim={1,1,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_CPU(0, CPU)} ; X = &_ ## X ;

#define ATMI_LPARM_CPU_1D(X,CPU,Y) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,1,1},.groupDim={1,1,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_CPU(0, CPU)} ; X = &_ ## X ;

#define ATMI_LPARM_1D(X,Y) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,1,1},.groupDim={64,1,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_LPARM_GPU_1D(X,GPU,Y) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,1,1},.groupDim={64,1,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_GPU(0, GPU)} ; X = &_ ## X ;
 
#define ATMI_LPARM_2D(X,Y,Z) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,Z,1},.groupDim={64,8,1},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;
 
#define ATMI_LPARM_3D(X,Y,Z,V) atmi_lparm_t * X ; atmi_lparm_t  _ ## X ={.gridDim={Y,Z,V},.groupDim={8,8,8},.group=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_required_groups=0,.required_groups=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=-1,.place=ATMI_PLACE_ANY(0)} ; X = &_ ## X ;

#define ATMI_STREAM(NAME) atmi_task_group_t * NAME; atmi_task_group_t _ ## NAME ={.ordered=ATMI_TRUE} ; NAME = &_ ## NAME ; 

#define ATMI_PROFILE(NAME) NAME = malloc(sizeof(atmi_tprofile_t));

#define ATMI_PROFILE_NEW(NAME) atmi_tprofile_t * NAME ; atmi_tprofile_t _ ## NAME ={.dispatch_time=0,.ready_time=0,.start_time=0,.end_time=0} ; NAME = &_ ## NAME;
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
/* String macros that look like an API, but actually implement feature by     */
/* calling a null kernel under specific conditions.                           */ 
/*----------------------------------------------------------------------------*/
#ifdef __cplusplus
#define _CPPSTRING_ "C" 
#endif
#ifndef __cplusplus
#define _CPPSTRING_ 
#endif
extern _CPPSTRING_ atmi_task_handle_t __sync_kernel_pif(atmi_lparm_t *lparm);

#define SYNC_STREAM(s) \
{ \
    ATMI_LPARM(__lparm_sync_kernel); \
    __lparm_sync_kernel->synchronous = ATMI_TRUE; \
    __lparm_sync_kernel->groupable = ATMI_TRUE; \
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

#define __ATMI_H__
#endif //__ATMI_H__
