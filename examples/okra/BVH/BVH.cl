#define SVM_DATA_STRUCT_OPENCL_DEVICE

#include "BVH.h"
#include "svm_data_struct.h"

#define NULL 0
#define _E 1

inline bool containsPoint( Sphere *s, double ox, double oy, double oz) {
    double dx = s->x - ox;
    double dy = s->y - oy;    
    double dz = 0;
    double radius2 = s->radius2;
    if((dx*dx + dy*dy + dz*dz) <= radius2)
        return true;
    else
        return false;
}

inline bool processLeaf(__global OutIdx *v, __global Sphere *LeafObjects, size_t n, double x, double y, double z)
{
    for (size_t i = 0; i < n; i++)
    {
        Sphere *s = &LeafObjects[i];
        if (containsPoint(s, x, y, z))
        {
            v->idx = s->idx;
        }
    }
    return true;
}

inline int computeResult()
{
	// [5][5] X [5][5] X [5][1] X [1][5] Matrix multiplication
	// Result is used to saturate LUMA
	int zz; 
	int count = 20;
	int c[5][5], sum = 0, result2[5][1], compute_result =0;
			
	for (zz = 1; zz < count; zz++) {
		int i,j,k,m=5, n=5, x=5, q=5;
		for(i = 0; i < m; i++)
		{ 
			for(j = 0; j < x; j++)
			{  
				sum=0;
				for(k = 0; k < n; k++)
				{
					sum = sum + (a_mat[i][k] * b_mat[k][j]);
				}
				c[i][j] = sum / zz;
			}
		}
		sum = 0;
		m=5; n=5; x=1; q=5;
		for(i = 0; i < m; i++)
		{ 
			for(j = 0; j < x; j++)
			{  
				sum=0;
				for(k = 0; k < n; k++)
				{
					sum = sum + (c[i][k] * c_mat[k][j]);
				}
				result2[i][j] = sum;
			}
		}
		m=1; n=5; x=1; q=5;
		sum = 0;
		for(i = 0; i < m; i++)
		{ 
			for(j = 0; j < x; j++)
			{  
				sum=0;
				for(k = 0; k < n; k++)
				{
					sum = sum + (d_mat[i][k] * result2[k][j]);
				}
				compute_result += sum;
			}
		}
	}
	return compute_result;
}
inline bool doesPointLieInsideBVH(__global BVH* node, double x, double y, double z)
{
	return ((x >= node->minX) && (y>= node->minY)  && (x <= node->maxX) && (y<= node->maxY));
}

/*
 * This kernel searches a set of points in a BVH.
 * Arguments:
 *		1. root node of the BVH.
 *		2. An array of points to be searched.
 *		3. An array of nodes pointers found in the search.
 */
__kernel void bvh_search(
			__global void *root_parm,
			__global float_3 *search_point,
			__global void* found_nodes_parm) 
{
	__global BVH *root = (__global BVH *)root_parm;
		
	int gid = get_global_id(0);	
	int init_id = gid;
	
	float x = search_point[init_id].x;
	float y = search_point[init_id].y;
	float z = search_point[init_id].z;
	__global OutIdx *found_nodes_temp = (__global OutIdx *)found_nodes_parm;
	__global OutIdx *list=found_nodes_temp+init_id;

    // Allocate traversal stack from thread-local memory,
    // and push NULL to indicate that there are no postponed nodes.
    //BVH *stack[64]={NULL};
   // BVH **stackPtr = stack;
    //stackPtr = NULL; // push
  //  *stackPtr++ = NULL;

    // Traverse nodes starting from the root.
    __global BVH* node = root;
    do
    {
        // Check each child node for overlap.
        __global BVH* childL = node->prev;
        __global BVH* childR = node->next;


        bool overlapL = doesPointLieInsideBVH(node->prev,x,y,z);
        bool overlapR = doesPointLieInsideBVH(node->next,x,y,z);

		
        // Query overlaps a leaf node => report collision.
        if (overlapL && childL->nleafObjects!=0)
			list->idx = childL->leafObjects[0].idx;// + computeResult();
		
        if (overlapR && childR->nleafObjects!=0)
            list->idx = childR->leafObjects[0].idx;// +computeResult();
		
		/*
		list->idx = (overlapL && childL->nleafObjects!=0) * childL->leafObjects[0].idx + (!(overlapL && childL->nleafObjects!=0)) * list->idx;
		list->idx = (overlapR && childR->nleafObjects!=0) * childR->leafObjects[0].idx + (!(overlapR && childR->nleafObjects!=0)) * list->idx;
		list->idx +=  (overlapL && childL->nleafObjects!=0) * computeResult();
		list->idx += 	(overlapR && childR->nleafObjects!=0) * computeResult();
		*/
        // Query overlaps an internal node => traverse.
        bool traverseL = (overlapL && !node->prev->nleafObjects);
        bool traverseR = (overlapR && !node->next->nleafObjects);
		
		#if 1
		node=NULL;
		//if (traverseL || traverseR)
		node = (traverseR) ? childR : node;
		 node = (traverseL) ? childL : node;
 		 

		#endif
		//node =(__global BVH*) ((traverseL)*(long) childL + (traverseR)* (long)childR+(1-traverseL-traverseR)*NULL);
		//node = (traverseL) ? childL : ((traverseR)? childR:NULL);
    }
    while (node);
}
