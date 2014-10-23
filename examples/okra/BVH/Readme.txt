1) To execute the exe please use following:

bvh.exe <0 or 1> <number of spheres>

0 builds BVH for CPU using malloc and executes 1M search points traversal on CPU
1 builds BVH using clSVMalloc and executes 1 M search points traversal on HSA

Total numbeer of spheres used =  <number of spheres> * 1024 * 1024

2) Search points range is defined in initialize_search_points function in HsaApp.cpp

3) Run as ./BVH 1 1
(where first 1 says run on HSA and second 1 gives size of nodes in Millions)
