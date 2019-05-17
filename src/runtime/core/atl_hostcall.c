 
/*   
 *   atl_hostcall.c: Implementation of Linked List Queue in c with 
 *              key search and key delete feature for ATMI to implement
 *              the hostcall feature. hostcall buffers are placed on
 *              this queue with push.  
 *
 *   Written by Greg Rodgers

MIT License

Copyright © 2019 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#include "hsa_ext_amd.h"
#include "hostcall.h"
#include "hostcall_service_id.h"
#include "atl_internal.h"

typedef struct atl_hostcall_element_s atl_hostcall_element_t;
struct atl_hostcall_element_s {
  hostcall_buffer_t * hcb;
  hostcall_consumer_t * consumer;
  void* key;
  atl_hostcall_element_t * next_ptr;
} ;

void            atl_hostcall_pop();
int             atl_hostcall_is_empty();
void            atl_hostcall_create();
int             atl_hostcall_size();
void            atl_hostcall_delete_by_key(void* key);

atl_hostcall_element_t * atl_hostcall_push(hostcall_buffer_t * buffer, hostcall_consumer_t * consumer, void*key);
atl_hostcall_element_t * atl_hostcall_get_front();
atl_hostcall_element_t * atl_hostcall_get_rear();
atl_hostcall_element_t * atl_hostcall_find_by_key(void* key);

/* Static "globals" for this module */
atl_hostcall_element_t * atl_hostcall_front;
atl_hostcall_element_t * atl_hostcall_rear;
int atl_hostcall_count;
 
void atl_hostcall_createq(){
  atl_hostcall_count = 0; 
  atl_hostcall_front = atl_hostcall_rear = NULL; 
}

int atl_hostcall_size() { return atl_hostcall_count ;}

atl_hostcall_element_t * atl_hostcall_push(hostcall_buffer_t * hcb, hostcall_consumer_t * consumer, void* key) {
  // FIXME , check rc of these mallocs
  if (atl_hostcall_rear == NULL) {
    atl_hostcall_rear = (atl_hostcall_element_t *) malloc(sizeof(atl_hostcall_element_t));
    atl_hostcall_front = atl_hostcall_rear;
  } else {
    atl_hostcall_element_t * new_rear = (atl_hostcall_element_t *) malloc(sizeof(atl_hostcall_element_t));
    atl_hostcall_rear->next_ptr = new_rear;
    atl_hostcall_rear = new_rear;
  }
  atl_hostcall_rear->next_ptr = NULL;
  atl_hostcall_rear->hcb      = hcb; 
  atl_hostcall_rear->key      = key;
  atl_hostcall_rear->consumer = consumer;
  atl_hostcall_count++;
  return atl_hostcall_rear;
}
 
void atl_hostcall_pop() {
  if (atl_hostcall_front  == NULL) {
    printf("\n Error: Trying to pop an element from empty queue");
    return;
  } else {
    if (atl_hostcall_front->next_ptr != NULL) {
      atl_hostcall_element_t * new_front = atl_hostcall_front->next_ptr;
      //printf("Freeing data at %p\n",(void*) atl_hostcall_front);
      free(atl_hostcall_front);
      atl_hostcall_front = new_front;
    } else {
      //printf("Freeing data at %p\n QUEUE NOW EMPTY",(void*) atl_hostcall_front);
      free(atl_hostcall_front);
      atl_hostcall_front = NULL;
      atl_hostcall_rear = NULL;
    }
    atl_hostcall_count--;
  }
}
 
atl_hostcall_element_t * atl_hostcall_get_front() {return atl_hostcall_front;}
atl_hostcall_element_t * atl_hostcall_get_rear() {return atl_hostcall_rear;}

/* Returns the rear data of the atl_hostcall */
hostcall_buffer_t * atl_hostcall_get_rear_data() {
  if ((atl_hostcall_front != NULL) && (atl_hostcall_rear != NULL))
    return(atl_hostcall_rear->hcb);
  else {
    hostcall_buffer_t* atl_hostcall_data_unknown;
    return atl_hostcall_data_unknown;
  }
}
 
/* Returns 1 if empty 0 otherwise */
int atl_hostcall_is_empty() {
  if ((atl_hostcall_front == NULL) && (atl_hostcall_rear == NULL))
    return(1);
  else
    return(0);
}

atl_hostcall_element_t  * atl_hostcall_find_by_key(void* key) {
  atl_hostcall_element_t * this_front = atl_hostcall_front;
  int reverse_counter = atl_hostcall_size();
  while (reverse_counter) {
    if (this_front->key == key)
       return this_front;
    this_front = this_front->next_ptr;
    reverse_counter--;
  }
  //printf("key not found\n");
  return NULL;
}

atl_hostcall_element_t  * atl_hostcall_find_by_index(int index) {
  atl_hostcall_element_t * this_front = atl_hostcall_front;
  int reverse_counter = atl_hostcall_size();
  while (reverse_counter) {
    if (reverse_counter == index)
       return this_front;
    this_front = this_front->next_ptr;
    reverse_counter--;
  }
  return NULL;
}

void atl_hostcall_delete_by_key(void* key) {
  atl_hostcall_element_t * this_front = atl_hostcall_front;
  atl_hostcall_element_t * last_front;
  int reverse_counter = atl_hostcall_size();
  while (reverse_counter) {
    if (this_front->key == key) {
      if (this_front == atl_hostcall_front) {
        atl_hostcall_pop();
      } else {
        last_front->next_ptr = this_front->next_ptr;
        //printf("DELETE: Freeing data at %p\n",(void*) this_front);
        free(this_front);
        atl_hostcall_count--;
      }
      return;
    }
    last_front = this_front;
    this_front = this_front->next_ptr;
    reverse_counter--;
  }
  return ;
}

static hostcall_buffer_t * atl_hostcall_create_buffer(unsigned int num_packets, 
	 hsa_amd_memory_pool_t finegrain_pool) {
    if (num_packets == 0) {
	printf("num_packets cannot be zero \n");
	abort();
    }
    auto size  = hostcall_get_buffer_size(num_packets);
    auto align = hostcall_get_buffer_alignment();
    void *newbuffer = NULL;
    hsa_status_t err = hsa_amd_memory_pool_allocate(finegrain_pool, size, 0, &newbuffer);
    if (!newbuffer) {
	    printf("call to  hsa_amd_memory_pool_allocate failed \n");
	    abort();
    }
    if (hostcall_initialize_buffer(newbuffer, num_packets) != HOSTCALL_SUCCESS) {
	    printf("call to  hostcall_initialize_buffer failed \n");
	    abort();
    }
    // printf("created hostcall buffer %p with %d packets \n", newbuffer, num_packets);
    return (hostcall_buffer_t *) newbuffer;
}

void handler_HOSTCALL_SERVICE_PRINTF(void *ignored, uint32_t service, uint64_t *payload) {
    *payload = *payload + 1;
    printf("=================== Printf Handler called\n");
}

void handler_HOSTCALL_SERVICE_MALLOC(void *ignored, uint32_t service, uint64_t *payload) {
	payload_t * p = (payload_t  *) payload;

    printf("===================malloc Handler called for service %d pl0:%ld \n",service,
		    payload[0]);
    payload[1] = 42; // Attempt a temporary LOOPTEST. 
    // The LOOPTeST IS TO TAKE result0 FROM FIRST hostcall_invoke and use it as input0 in 2nd call to invoke.
    // We should see this handler get a 42. 
    if (payload[0] == 42 ) {
	    printf("temporary LOOPTeST is success the 42 was returned in a 2nd call to hostall\n");
	    abort();
    }
}
void handler_HOSTCALL_SERVICE_FREE(void *ignored, uint32_t service, uint64_t *payload) {
}
void handler_HOSTCALL_SERVICE_DEMO(void *ignored, uint32_t service, uint64_t *payload) {
}

// ----   External functions start here  -----
unsigned long atl_hostcall_assign_buffer(unsigned int minpackets, 
		hsa_queue_t * this_Q, hsa_amd_memory_pool_t finegrain_pool) {
  void * hostcall_ptrs;

    atl_hostcall_element_t * llq_elem ;
    llq_elem  = atl_hostcall_find_by_key((void*) this_Q);
    if (!llq_elem) {
       //  For now, we create one bufer and one consumer per hsa queue
       hostcall_buffer_t * hcb  = atl_hostcall_create_buffer(minpackets,
		 finegrain_pool) ;
       
       hostcall_consumer_t * c;
       c = hostcall_create_consumer();
       hostcall_register_service(c,HOSTCALL_SERVICE_PRINTF, handler_HOSTCALL_SERVICE_PRINTF, nullptr);
       hostcall_register_service(c,HOSTCALL_SERVICE_MALLOC, handler_HOSTCALL_SERVICE_MALLOC, nullptr);
       hostcall_register_service(c,HOSTCALL_SERVICE_FREE, handler_HOSTCALL_SERVICE_FREE, nullptr);
       hostcall_register_service(c,HOSTCALL_SERVICE_DEMO, handler_HOSTCALL_SERVICE_DEMO, nullptr);
       hostcall_register_buffer(c,hcb);
       allow_access_to_all_gpu_agents(hcb);
       llq_elem = atl_hostcall_push( hcb , c, (void*) this_Q);
       hostcall_launch_consumer(c);
    }
    return (unsigned long) llq_elem->hcb;
}

hsa_status_t atl_hostcall_init(hsa_agent_t * agent) {
   void atl_hostcall_createq();
   return HSA_STATUS_SUCCESS;
}
