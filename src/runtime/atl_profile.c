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
#include "atl_profile.h"
#include "atl_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FIXME: How many profiling agents in total do we need to support? 
// There should be one profiling agent per device queue. 
// 128 should be large enough for single GPU systems.
atmi_profiling_agent_t* atmi_profiling_agent_list[128];

char profiling_fname[32] = {'\0'};

extern struct timespec context_init_time;

static atmi_profiling_buffer_t* atmi_profiling_malloc_buffer();

int atmi_profiling_init()
{
    uint64_t start_time_ns;
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC_RAW,&start_time);
    start_time_ns = get_nanosecs(context_init_time, start_time);
    sprintf(profiling_fname, "_profiling_%lu_", start_time_ns);
    return 0;
}

int atmi_profiling_agent_init(int tid)
{
    atmi_profiling_agent_t *agt;

    agt = (atmi_profiling_agent_t *)malloc(sizeof(atmi_profiling_agent_t));
    agt->total_nb_tasks = 0;
    agt->buffer = atmi_profiling_malloc_buffer(); 
    agt->cur_buffer = agt->buffer;
    atmi_profiling_agent_list[tid] = agt;
    return 0;
}

int atmi_profiling_agent_fini(int tid)
{
    atmi_profiling_agent_t *agt = atmi_profiling_agent_list[tid];
    atmi_profiling_buffer_t *buffer, *next_buffer;
    buffer = agt->buffer;
    while (buffer != NULL) {
        next_buffer = buffer->next_buffer;
        free(buffer);
        buffer = next_buffer;
    }
    free(agt);
    return 0;
}

int atmi_profiling_record(int tid, atmi_tprofile_t *p, char *name)  
{
    atmi_profiling_agent_t *agt = atmi_profiling_agent_list[tid];
    if (agt->cur_buffer->nb_tasks >= ATMI_PROFILING_BUFFER_SIZE) {
        atmi_profiling_buffer_t * buffer = atmi_profiling_malloc_buffer();
        agt->cur_buffer->next_buffer = buffer;
        agt->cur_buffer = buffer;
    }
    agt->cur_buffer->tasks[agt->cur_buffer->nb_tasks].profile_info = p;
    agt->cur_buffer->tasks[agt->cur_buffer->nb_tasks].name = name;
    agt->cur_buffer->nb_tasks ++;
    agt->total_nb_tasks ++; 
    return 0;
}

int atmi_profiling_output(int tid)
{
    atmi_profiling_agent_t *agt = atmi_profiling_agent_list[tid];
    atmi_profiling_buffer_t *buffer = agt->buffer;
    int i;
    char tfname[32];
    char tbuffer[4];
    FILE *pFile;
    strcpy(tfname, profiling_fname);
    sprintf(tbuffer, "%d", tid);
    strcat(tfname, tbuffer);
    pFile = fopen(tfname, "w");
    fprintf(pFile, "%d\n", tid);
    while (buffer != NULL) {
        for (i = 0; i < buffer->nb_tasks; i++) {
            fprintf(pFile, "%s\t%lu\t%lu\n", buffer->tasks[i].name, buffer->tasks[i].profile_info->start_time, buffer->tasks[i].profile_info->end_time);
        }
        buffer = buffer->next_buffer;
    }
    fclose(pFile);
    return 0; 
}

static atmi_profiling_buffer_t* atmi_profiling_malloc_buffer()
{
    atmi_profiling_buffer_t *buffer = (atmi_profiling_buffer_t *)malloc(sizeof(atmi_profiling_buffer_t));
    if (buffer == NULL) {
        return NULL;
    }
    buffer->nb_tasks = 0;
    buffer->next_buffer = NULL;
    return buffer;
}
