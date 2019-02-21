/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
#ifndef __ATL_DATA__
#define __ATL_DATA__
#include "atmi.h"
#include <hsa.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <mutex>
namespace core {
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

enum {
    ATMI_H2D = 0,
    ATMI_D2H = 1,
    ATMI_D2D = 2,
    ATMI_H2H = 3
};

hsa_agent_t get_mem_agent(atmi_mem_place_t place);
hsa_agent_t get_compute_agent(atmi_place_t place);
} // namespace core
#endif // __ATL_DATA__
