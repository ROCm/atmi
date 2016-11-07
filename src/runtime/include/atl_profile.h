/*
MIT License 

Copyright © 2016 Advanced Micro Devices, Inc.  

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
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
