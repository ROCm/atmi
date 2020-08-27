/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef INCLUDE_ATMI_KL_H_
#define INCLUDE_ATMI_KL_H_

#include <atmi.h>

extern void atmid_task_launch(atmi_lparm_t *lp, unsigned long kernel_id,
                              void *args_region,
                              unsigned long args_region_size);

#endif  // INCLUDE_ATMI_KL_H_
