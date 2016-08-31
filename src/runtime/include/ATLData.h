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
#ifndef __ATL_DATA__
#define __ATL_DATA__
#include "atmi.h"
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <mutex>
class ATLData {
    public: 
        ATLData(void *ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t type) : 
            _ptr(ptr), _host_alias_ptr(NULL), _size(size), _place(place), _arg_type(type) {}
        
        ATLData(void *ptr, void *host_ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t type) : 
            _ptr(ptr), _host_alias_ptr(host_ptr), _size(size), _place(place), _arg_type(type) {}

        void *getPtr() const { return _ptr; }
        void *getHostAliasPtr() const { return _host_alias_ptr; }
        size_t getSize() const { return _size; }
        atmi_mem_place_t getPlace() const { return _place; }
        atmi_arg_type_t getArgType() const { return _arg_type; }
    private:
       // make this a vector of pointers? 
       void *_ptr;
       void *_host_alias_ptr;
       size_t _size;
       atmi_mem_place_t _place;
       atmi_arg_type_t _arg_type;
};
//---
struct ATLMemoryRange {
    const void * _basePointer;
    const void * _endPointer;
    ATLMemoryRange(const void *basePointer, size_t sizeBytes) :
        _basePointer(basePointer), _endPointer((const unsigned char*)basePointer + sizeBytes - 1) {}
};

// Functor to compare ranges:
struct ATLMemoryRangeCompare {
    // Return true is LHS range is less than RHS - used to order the ranges 
    bool operator()(const ATLMemoryRange &lhs, const ATLMemoryRange &rhs) const
    {
        return lhs._endPointer < rhs._basePointer;
    }
};

//-------------------------------------------------------------------------------------------------
// This structure tracks information for each pointer.
// Uses memory-range-based lookups - so pointers that exist anywhere in the range of hostPtr + size 
// will find the associated ATLPointerInfo.
// The insertions and lookups use a self-balancing binary tree and should support O(logN) lookup speed.
// The structure is thread-safe - writers obtain a mutex before modifying the tree.  Multiple simulatenous readers are supported.
class ATLPointerTracker {
typedef std::map<ATLMemoryRange, ATLData *, ATLMemoryRangeCompare> MapTrackerType;
public:
    void insert(void *pointer, ATLData *data);
    void remove(void *pointer);
    ATLData *find(const void *pointer) ;
private:
    MapTrackerType  _tracker;
    std::mutex      _mutex;
    //std::shared_timed_mutex _mut;
};

enum {
    ATMI_H2D = 0,
    ATMI_D2H = 1,
    ATMI_D2D = 2,
    ATMI_H2H = 3
};

#endif // __ATL_DATA__
