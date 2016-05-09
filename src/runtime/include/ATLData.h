#ifndef __ATL_DATA__
#define __ATL_DATA__
#include "atmi.h"
#include <stdio.h>
#include <stdlib.h>
class ATLData {
    public: 
        ATLData(void *ptr, size_t size, atmi_mem_place_t place) : 
            _ptr(ptr), _host_alias_ptr(NULL), _size(size), _place(place) {}
        
        ATLData(void *ptr, void *host_ptr, size_t size, atmi_mem_place_t place) : 
            _ptr(ptr), _host_alias_ptr(host_ptr), _size(size), _place(place) {}

        void *getPtr() { return _ptr; }
        void *getHostAliasPtr() { return _host_alias_ptr; }
        size_t getSize() { return _size; }
        atmi_mem_place_t getPlace() { return _place; }
    private:
       void *_ptr;
       void *_host_alias_ptr;
       size_t _size;
       atmi_mem_place_t _place;
};

#endif // __ATL_DATA__
