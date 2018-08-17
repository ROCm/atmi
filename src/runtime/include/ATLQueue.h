/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __ATL__QUEUE__
#define __ATL__QUEUE__

#include "hsa.h"
#include "atmi.h"
class ATLQueue {
    public:
        ATLQueue(hsa_queue_t *q, atmi_place_t p = ATMI_PLACE_ANY(0)) :
                _queue(q), _place(p) {}
        hsa_queue_t *getQueue() const { return _queue; }
        atmi_place_t getPlace() const { return _place; }

        hsa_status_t setPlace(atmi_place_t place);
    protected:
        hsa_queue_t*    _queue;
        atmi_place_t    _place;
};

class ATLCPUQueue : public ATLQueue {
    public:
        ATLCPUQueue(hsa_queue_t *q, atmi_place_t p = ATMI_PLACE_ANY_CPU(0)) : ATLQueue(q, p) {}
        hsa_status_t setPlace(atmi_place_t place);
};

class ATLGPUQueue : public ATLQueue {
    public:
        ATLGPUQueue(hsa_queue_t *q, atmi_place_t p = ATMI_PLACE_ANY_GPU(0)) : ATLQueue(q, p) {}
        hsa_status_t setPlace(atmi_place_t place);
};

#endif // __ATL__QUEUE__
