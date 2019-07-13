/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef ATMI_BINDTHREAD_H
#define ATMI_BIND_THREAD_H

#include "atl_internal.h"
int atmi_cpu_bindthread(int cpu_index);
atmi_status_t set_thread_affinity(int id);

#endif /* ATMI_BIND_THREAD_H */
