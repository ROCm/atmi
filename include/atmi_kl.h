#ifndef __ATMI_KL_H__
#define __ATMI_KL_H__

/* Unsigned.  */
typedef unsigned int atmi_uint32_t;
typedef unsigned long int atmi_uint64_t;

typedef struct atmi_kernel_dispatch_packet_s {
    /**
     * Size in bytes of private memory allocation request (per work-item).
     */
    atmi_uint32_t private_segment_size;

    /**
     * Size in bytes of group memory allocation request (per work-group). Must not
     * be less than the sum of the group memory used by the kernel (and the
     * functions it calls directly or indirectly) and the dynamically allocated
     * group segment variables.
     */
    atmi_uint32_t group_segment_size;

    /**
     * Opaque handle to a code object that includes an implementation-defined
     * executable code for the kernel.
     */
    atmi_uint64_t kernel_object;

    /**
     * Pointer to a buffer containing the kernel arguments. May be NULL.
     *
     * The buffer must be allocated using ::hsa_memory_allocate, and must not be
     * modified once the kernel dispatch packet is enqueued until the dispatch has
     * completed execution.
     */
    void *kernarg_address;

    /**
     * Reserved. Must be 0.
     */
    atmi_uint64_t reserved2;

    /**
     * Signal used to indicate completion of the job. The application can use the
     * special signal handle 0 to indicate that no signal is used.
     */
    atmi_uint64_t completion_signal;
} atmi_kernel_dispatch_packet_t;


typedef struct atmi_klist_s atmi_klist_t;
struct atmi_klist_s { 
    int num_kernel_packets;
    int num_queues;
    atmi_uint64_t *queues;
    atmi_kernel_dispatch_packet_t *kernel_packets;
};

/*----------------------------------------------------------------------------*/
typedef struct atmi_klparm_s {
    unsigned long    gridDim[3];     /* # of global threads for each dimension */
    unsigned long    groupDim[3];    /* Thread group size for each dimension   */
    atmi_stream_t*   stream;         /* Group for this task, Default= NULL     */
    boolean          waitable;       /* Create signal for task, default = F    */
    boolean          synchronous;    /* Async or Sync,  default = F (async)    */
    int              acquire_scope;  /* Memory model, default = 2              */
    int              release_scope;  /* Memory model, default = 2              */
    int              num_required;   /* # of required parent tasks, default 0  */
    atmi_task_t**    requires;       /* Array of required parent tasks         */
    int              num_needs_any;  /* # needed parents, only 1 must complete */
    atmi_task_t**    needs_any;      /* Array of needed parent tasks           */
    boolean          profilable;     /* Points to tprofile if metrics desired  */ 
    int              atmi_id;        /* Constant that PIFs can check for       */
    int              kernel_id;
    void             *prevTask;
} atmi_klparm_t ;

#endif
