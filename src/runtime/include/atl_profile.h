/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef ATMI_PROFILING_H
#define ATMI_PROFILING_H

#include "atmi.h"
#include "atl_rt.h"

//#define ATMI_HAVE_PROFILE

#define ATMI_PROFILING_BUFFER_SIZE	4096

typedef struct atmi_profiling_task_s {
    char*			name;
    atmi_tprofile_t	*profile_info;
} atmi_profiling_task_t;

typedef struct atmi_profiling_buffer_s {
    uint32_t						nb_tasks;
    struct atmi_profiling_task_s	tasks[ATMI_PROFILING_BUFFER_SIZE];
    struct atmi_profiling_buffer_s	*next_buffer;
} atmi_profiling_buffer_t;

typedef struct atmi_profiling_agent_s {
    uint32_t						tid;
    struct atmi_profiling_buffer_s	*buffer;
    struct atmi_profiling_buffer_s	*cur_buffer;
    uint64_t						total_nb_tasks;
} atmi_profiling_agent_t;

extern atmi_profiling_agent_t* atmi_profiling_agent_list[128];

extern char profiling_fname[32];

int atmi_profiling_init();

int atmi_profiling_agent_init(int tid);

int atmi_profiling_agent_fini(int tid);

int atmi_profiling_record(int tid, atmi_tprofile_t *p, char *name);

int atmi_profiling_output(int tid);
#endif /* ATMI_PROFILING_H */
