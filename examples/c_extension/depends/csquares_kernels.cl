/*===--------------------------------------------------------------------------
 *              ATMI (Asynchronous Task and Memory Interface)
 *
 * This file is distributed under the MIT License. See LICENSE.txt for details.
 *===------------------------------------------------------------------------*/
/*
   File:  csquares.cl 

   3 Kernels for csquares.cpp. 
   This is not intended to be efficient.  
   It is just a simple demo of dependencies. 
*/
/*  Parent kernel initializes input array */
__kernel void init_kernel_gpu(__global int *in) {
	int i = get_global_id(0);
	in[i] = (int)  i;
}

/*  Middle children calculate squares for even numbers */
__kernel void even_squares_kernel_gpu(
   __global const int *in , __global int *out) 
{
	int i = get_global_id(0)*2;
	out[i] = in[i] * in[i];
}

/*  The last child calculate squares for odd numbers 
    using squares from even numbers because.
        (X-1)**2 = X**2 - 2X + 1 
    so  X**2 = ((X-1)**2) + 2X - 1
*/
__kernel void odd_squares_kernel_gpu(__global int *out) 
{
	int i = (get_global_id(0)*2) + 1;
	out[i] = out[i-1] + (2*i) - 1; 
}
