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

typedef struct atmi_klparm_s atmi_klparm_t;
struct atmi_klparm_s { 
    int ndim;                  /* default = 1 */
    unsigned long           gdims[3];       /* # of global threads for each dimension */
    unsigned long           ldims[3];       /* Thread group size for each dimension   */
    int stream;                /* default = -1 , synchrnous */
    int barrier;               /* default = SNK_UNORDERED */
    int acquire_fence_scope;   /* default = 2 */
    int release_fence_scope;   /* default = 2 */
    int kernel_id;
    void *prevTask;
};

#endif
