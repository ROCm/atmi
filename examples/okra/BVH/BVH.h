#ifndef BVH_H
#define BVH_H

#include "Sphere.h"
#include "mem_mgr.h"

#ifndef SVM_DATA_STRUCT_OPENCL_DEVICE
#define __global 
#endif

typedef enum _Axis {X,Y,Z} Axis;

typedef struct _float_3
{
    double x;
    double y;
    double z;
} float_3;

typedef struct _OutIdx
{
    intptr_t idx;
} OutIdx;

typedef struct _BVH
{
    double minX;
    double maxX;
    double minY;
    double maxY;
    double minZ;
    double maxZ;
    __global struct _BVH *prev;
    __global struct _BVH *next;
    __global struct _BVH *parent;		
    __global Sphere *leafObjects;
    size_t nleafObjects;
} BVH;

void buildBVH(BVH *b, Sphere *spherelist, BVH *lparent, size_t count, Axis axisid, mem_mgr m, mem_mgr objdata);
//void buildBVH(BVH *b, Sphere *spherelist, BVH *lparent, size_t start, size_t end, Axis axisid, mem_mgr m, mem_mgr objdata);
void destroyBVH(BVH *b);
void traverseBVHRecursive(BVH *b, OutIdx *list, size_t *start, double x,double y,double z);
void traverseBVHIterative(BVH *b, OutIdx *list, size_t *start, double x,double y,double z);

#ifdef SVM_DATA_STRUCT_OPENCL_DEVICE
// GPU Compute matrix
__constant  int a_mat[5][5] = {	1, 2, 3, 4, 5,
							6, 7, 8, 9, 10,
							11, 12, 13, 14, 15,
							17, 18, 19, 20, 21,
							22, 23, 24, 25, 26
						  };
__constant  int b_mat[5][5] = {	1, 2, 3, 4, 5,
							6, 7, 8, 9, 10,
							11, 12, 13, 14, 15,
							11, 12, 13, 14, 15,
							11, 12, 13, 14, 15
						  };
__constant  int c_mat[5][1] = {27,
						  28,
						  29,
						  30
						 };
__constant int d_mat[1][5] = {27,28,29,30};
#else
// GPU Compute matrix
const int a_mat[5][5] = {	1, 2, 3, 4, 5,
							6, 7, 8, 9, 10,
							11, 12, 13, 14, 15,
							17, 18, 19, 20, 21,
							22, 23, 24, 25, 26
						  };
const int b_mat[5][5] = {	1, 2, 3, 4, 5,
							6, 7, 8, 9, 10,
							11, 12, 13, 14, 15,
							11, 12, 13, 14, 15,
							11, 12, 13, 14, 15
						  };
const int c_mat[5][1] = {27,
						  28,
						  29,
						  30
						 };
const int d_mat[1][5] = {27,28,29,30};
#endif



#endif
