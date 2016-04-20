#include "ATLQueue.h"
#include "atmi.h"
#include "hsa_ext_amd.h"

bool equalsPlace(const atmi_place_t &l, const atmi_place_t &r) {
    bool val = false;
    if(l.node_id == r.node_id && l.cpu_set == r.cpu_set && l.gpu_set == r.gpu_set)
        val = true;
    return val;
}

hsa_status_t ATLGPUQueue::setPlace(atmi_place_t place) {
    hsa_status_t val = HSA_STATUS_SUCCESS;
    if(!equalsPlace(_place, place)) {
        _place = place;
        val = hsa_amd_queue_cu_set_mask(_queue, 2, (uint32_t *)&(_place.gpu_set));
    }
    return val;
}

hsa_status_t ATLCPUQueue::setPlace(atmi_place_t place) {
    hsa_status_t val = HSA_STATUS_SUCCESS;
    if(!equalsPlace(_place, place)) {
        _place = place;
        // change pthread-to-core binding based on cpu_set. If number of bits that
        // are set on cpu_set is >1 then choose the first non-zero bit and place
        // the thread on that core. 
        // TODO: Any other scheduling algorithms based on load, task group
        // annotations, and so on...
    }
    return val;
}

