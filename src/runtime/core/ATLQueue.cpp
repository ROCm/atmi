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
#include "ATLQueue.h"
#include "atmi.h"
#include "hsa_ext_amd.h"

bool equalsPlace(const atmi_place_t &l, const atmi_place_t &r) {
    bool val = false;
    if(l.node_id == r.node_id && l.type == r.type && l.device_id == r.device_id && l.cu_mask == r.cu_mask)
        val = true;
    return val;
}

hsa_status_t ATLGPUQueue::setPlace(atmi_place_t place) {
    hsa_status_t val = HSA_STATUS_SUCCESS;
    if(!equalsPlace(_place, place)) {
        _place = place;
        val = hsa_amd_queue_cu_set_mask(_queue, 2, (uint32_t *)&(_place.cu_mask));
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

