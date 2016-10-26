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
