/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef SRC_RUNTIME_INCLUDE_ATL_BINDTHREAD_H_
#define SRC_RUNTIME_INCLUDE_ATL_BINDTHREAD_H_

#include "atl_internal.h"
int atmi_cpu_bindthread(int cpu_index);
atmi_status_t set_thread_affinity(int id);

#endif  // SRC_RUNTIME_INCLUDE_ATL_BINDTHREAD_H_
