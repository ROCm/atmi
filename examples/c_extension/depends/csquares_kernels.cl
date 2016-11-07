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
