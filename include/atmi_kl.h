#ifndef __ATMI_KL_H__
#define __ATMI_KL_H__

#define MAX_NUM_KERNELS 1024
typedef struct atmi_kernel_packet_s atmi_kernel_packet_t;
struct atmi_kernel_packet_s {
    unsigned char resevered[64];
};

typedef struct atmi_task_impl_s {
    unsigned long int signal;
    unsigned char reserved[424];
} atmi_task_impl_t;

typedef struct atmi_klist_s atmi_klist_t;
struct atmi_klist_s { 
    int num_kernel_packets;
    int num_queues;
    void *cpu_kernarg_heap;
    int cpu_kernarg_offset;
    void *gpu_kernarg_heap;
    int gpu_kernarg_offset;
    unsigned long int queues[2];
    unsigned long int worker_sig;
    atmi_kernel_packet_t *kernel_packets;
    atmi_kernel_packet_t *kernel_packets_heap;
    int kernel_packets_offset;
    void *tasks;
};

extern atmi_klist_t atmi_klist[];

/*----------------------------------------------------------------------------*/
typedef struct atmi_klparm_s {
    unsigned long    gridDim[3];     /* # of global threads for each dimension */
    unsigned long    groupDim[3];    /* Thread group size for each dimension   */
    atmi_task_group_t*   stream;         /* Group for this task, Default= NULL     */
    boolean          groupable;       /* Create signal for task, default = F    */
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
    atmi_task_handle_t  prevTask;
} atmi_klparm_t ;

#define ATMI_KLPARM_1D(X,Y,thisTask) atmi_klparm_t * X ; atmi_klparm_t  _ ## X ={.gridDim={Y},.groupDim={64},.stream=NULL,.groupable=ATMI_FALSE,.synchronous=ATMI_FALSE,.acquire_scope=2,.release_scope=2,.num_required=0,.requires=NULL,.num_needs_any=0,.needs_any=NULL,.profilable=ATMI_FALSE,.atmi_id=ATMI_VRM,.kernel_id=1,.place=ATMI_PLACE_ANY(0),.task_info=NULL,.prevTask=thisTask} ; X = &_ ## X ; 

#endif
