#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include "okra.h"

typedef okra_status_t (*okra_get_context_func_t)(okra_context_t**);
typedef okra_status_t (*okra_kernel_create_from_binary_func_t)(okra_context_t *, const char *, size_t , const char *, okra_kernel_t **);
typedef okra_status_t (*okra_push_pointer_func_t)(okra_kernel_t* , void* );
typedef okra_status_t (*okra_push_int_func_t)(okra_kernel_t* , int );
typedef okra_status_t (*okra_push_long_func_t)(okra_kernel_t* , long );
typedef okra_status_t (*okra_execute_kernel_func_t)(okra_context_t*, okra_kernel_t* , okra_range_t* );
typedef okra_status_t (*okra_clear_args_func_t)(okra_kernel_t* );
typedef okra_status_t (*okra_dispose_kernel_func_t)(okra_kernel_t* );
typedef okra_status_t (*okra_dispose_context_func_t)(okra_context_t* );

okra_get_context_func_t      			_okra_get_context;
okra_kernel_create_from_binary_func_t   _okra_kernel_create_from_binary;
okra_push_pointer_func_t    			_okra_push_pointer;
okra_push_int_func_t                _okra_push_int;
okra_push_long_func_t                _okra_push_long;
okra_execute_kernel_func_t    			_okra_execute_kernel;
okra_clear_args_func_t       			_okra_clear_args;
okra_dispose_kernel_func_t      		_okra_dispose_kernel;
okra_dispose_context_func_t       		_okra_dispose_context;

struct st {
   char *src;
   char *dst;
   uint32_t size ;
} *sample_object ;

#define LIST_SIZE 1024*1024
/**
 * @brief Example program to illustrate how Hsa Core Api could be
 * used to compile and dispatch a kernel.
 */
int main(int argc, char **argv) {
  int index;
  okra_status_t status;
  okra_context_t *context;
  okra_kernel_t *kernel = NULL;
  okra_range_t range;
  size_t size = 1;
  const char* fileName = "VectorCopy.brig";
  const char* entryPoint = "&__OpenCL_test_kernel";
  const char* pfile;
  void *args;
  void *okraLib;
  char *src_list;
  char *dst_list;
  uint32_t list_size = LIST_SIZE;
  uint32_t buff_size = list_size * sizeof(uint32_t);

  printf("Running VectorCopy sample using dlopen, CreateKernelFromBinary()\n");

  okraLib = dlopen("libokra_x86_64.so", RTLD_LAZY);
  if (okraLib == NULL) {
    printf("...unable to load libokra_x86_64.so\n");
    return -1;
  }
  _okra_get_context          	   = (okra_get_context_func_t)dlsym(okraLib, "okra_get_context");
  _okra_kernel_create_from_binary  = (okra_kernel_create_from_binary_func_t)dlsym(okraLib, "okra_create_kernel_from_binary");
  _okra_push_pointer               = (okra_push_pointer_func_t)dlsym(okraLib, "okra_push_pointer");
  _okra_push_int                   = (okra_push_int_func_t)dlsym(okraLib, "okra_push_int");
  _okra_push_long                  = (okra_push_long_func_t)dlsym(okraLib, "okra_push_long");
  _okra_execute_kernel             = (okra_execute_kernel_func_t)dlsym(okraLib, "okra_execute_kernel");
  _okra_clear_args                 = (okra_clear_args_func_t)dlsym(okraLib, "okra_clear_args");
  _okra_dispose_kernel             = (okra_dispose_kernel_func_t)dlsym(okraLib, "okra_dispose_kernel");
  _okra_dispose_context            = (okra_dispose_context_func_t)dlsym(okraLib, "okra_dispose_context");

  /* Intialize src pointer */
  src_list = (char *)malloc(buff_size);
  assert(src_list != NULL);
  //for (index = 0; index < list_size; index++)
  //  *(uint32_t *)((uint32_t *)src_list + index) = index;

  /* Intialize dst pointer */
  dst_list = (char *)malloc(buff_size);
  assert(dst_list != NULL);
  memset(dst_list, 0, buff_size);
  memset(src_list, 1, buff_size);
  sample_object =(struct st *) malloc(sizeof (struct st));
  sample_object->size = LIST_SIZE;
  memcpy (&sample_object->dst,&dst_list, sizeof(void*));
  memcpy (&sample_object->src,&src_list, sizeof(void*));
  args = (void *)sample_object;

  status = _okra_get_context(&context);
  if (status != OKRA_SUCCESS) {
     printf( "...unable to create context\n");
     return -1;
  }

  pfile = (const char *) fopen (fileName, "rb");
  status = _okra_kernel_create_from_binary(context, pfile, size, entryPoint, &kernel);
  if (status != OKRA_SUCCESS) {
     printf( "...unable to create Kernel \n");
     return -1;
  }

  _okra_clear_args(kernel);
#ifdef DUMMY_ARGS
       //This flags should be set if HSA_HLC_Stable is used
        // This is because the high level compiler generates 6 extra args
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
        okra_push_pointer(kernel, NULL);
#endif   
  _okra_push_pointer(kernel, dst_list);
  _okra_push_pointer(kernel, src_list);
//  _okra_push_long(kernel, 1024*1024);

 /* set launch dimensions */
  range.dimension = 1;
  range.global_size[0] = 1024*1024;
  range.group_size[0] = 256;
  status = _okra_execute_kernel(context, kernel, &range);
  if (status != OKRA_SUCCESS) {
     printf( "Failed to launch kernel \n");
     return -1;
  }

  /* Validate the result post dispatch, and print the differences if any */
  if (memcmp(src_list, dst_list, list_size) != 0) {
    printf ("Vector Copy Failed \n");
  } else {
    printf ("Vector Copy Passed \n");
  }
  free(dst_list);
  free(src_list);
  free((void *)sample_object);

  _okra_dispose_kernel(kernel);
  _okra_dispose_context(context);

}
