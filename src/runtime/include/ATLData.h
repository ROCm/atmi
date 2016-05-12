#ifndef __ATL_DATA__
#define __ATL_DATA__
#include "atmi.h"
#include <stdio.h>
#include <stdlib.h>
class ATLData {
    public: 
        ATLData(void *ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t type) : 
            _ptr(ptr), _host_alias_ptr(NULL), _size(size), _place(place), _arg_type(type) {}
        
        ATLData(void *ptr, void *host_ptr, size_t size, atmi_mem_place_t place, atmi_arg_type_t type) : 
            _ptr(ptr), _host_alias_ptr(host_ptr), _size(size), _place(place), _arg_type(type) {}

        void *getPtr() { return _ptr; }
        void *getHostAliasPtr() { return _host_alias_ptr; }
        size_t getSize() { return _size; }
        atmi_mem_place_t getPlace() { return _place; }
        atmi_arg_type_t getArgType() { return _arg_type; }
    private:
       // make this a vector of pointers? 
       void *_ptr;
       void *_host_alias_ptr;
       size_t _size;
       atmi_mem_place_t _place;
       atmi_arg_type_t _arg_type;
};

#endif // __ATL_DATA__
