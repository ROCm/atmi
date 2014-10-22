#include <algorithm>
#include <iostream>
#include "BVH.h"

static bool bvhsortx (Sphere s,Sphere t) { return (s.x<t.x); }
static bool bvhsorty (Sphere s,Sphere t) { return (s.y<t.y); }
static bool bvhsortz (Sphere s,Sphere t) { return (s.z<t.z); }
static bool bvhcompx (Sphere s,double val) { return (s.x<val); }
static bool bvhcompy (Sphere s,double val) { return (s.y<val); }
static bool bvhcompz (Sphere s,double val) { return (s.z<val); }
inline Axis NextAxis(Axis d) { return Axis((d+1)%(Z+1)); }

void buildBVH(BVH *b, Sphere *spherelist, BVH *lparent, size_t count, Axis axisid, mem_mgr nodeData, mem_mgr objData)
{
    size_t center;
    size_t loop;

    // save off the parent BVHGObj Node
    b->parent = lparent;

    // Early out check due to bad data
    // If the list is empty then we have no BVHGObj, or invalid parameters are passed in
    if (!spherelist|| (count == 0))
    {
        b->minX = 0;
        b->maxX = 0;
        b->minY = 0;
        b->maxY = 0;
        b->minZ = 0;
        b->maxZ = 0;
        b->prev = NULL;
        b->next = NULL;
        b->leafObjects = NULL;
        return;
    }

    // Check if we’re at our LEAF node, and if so, save the objects and stop recursing. Also store the min/max for the leaf node and update the parent appropriately
    if (count <= 2)
    {
        // We need to find the aggregate min/max for all 4 remaining objects
        // Start by recording the min max of the first object to have a starting point, then we’ll loop through the remaining
        b->minX = spherelist[0].x - spherelist[0].radius;
        b->maxX = spherelist[0].x + spherelist[0].radius;
        b->minY = spherelist[0].y - spherelist[0].radius;
        b->maxY = spherelist[0].y + spherelist[0].radius;
        b->minZ = spherelist[0].z - spherelist[0].radius;
        b->maxZ = spherelist[0].z + spherelist[0].radius;

        // once we reach the leaf node, we must set prev/next to NULL to signify the end
        b->prev = NULL;
        b->next = NULL;

        // at the leaf node we store the remaining objects, so initialize a list
        b->leafObjects = (Sphere*)objData->mem_malloc(objData, count*sizeof(Sphere));
        b->nleafObjects = count;

        // loop through all the objects to add them to our leaf node, and calculate the min/max values as we go
        for (loop = 0; loop < count; loop++)
        {
            // test min X and max X against the current bounding volume
            if ((spherelist[loop].x - spherelist[loop].radius) < b->minX)
                b->minX = (spherelist[loop].x - spherelist[loop].radius);
            if ((spherelist[loop].x + spherelist[loop].radius) > b->maxX)
                b->maxX = (spherelist[loop].x + spherelist[loop].radius);

            // Update the leaf node’s parent if appropriate with the min/max
            if (b->parent != NULL && b->minX < b->parent->minX)
                b->parent->minX = b->minX;
            if (b->parent != NULL && b->maxX > b->parent->maxX)
                b->parent->maxX = b->maxX;

            // test min Y and max Y against the current bounding volume
            if ((spherelist[loop].y - spherelist[loop].radius) < b->minY)
                b->minY = (spherelist[loop].y - spherelist[loop].radius);
            if ((spherelist[loop].y + spherelist[loop].radius) > b->maxY)
                b->maxY = (spherelist[loop].y + spherelist[loop].radius);

            // Update the leaf node’s parent if appropriate with the min/max
            if (b->parent != NULL && b->minY < b->parent->minY)
                b->parent->minY = b->minY;
            if (b->parent != NULL && b->maxY > b->parent->maxY)
                b->parent->maxY = b->maxY;

            // test min Z and max Z against the current bounding volume
            if ( (spherelist[loop].z - spherelist[loop].radius) < b->minZ )
                b->minZ = (spherelist[loop].z - spherelist[loop].radius);
            if ( (spherelist[loop].z + spherelist[loop].radius) > b->maxZ )
                b->maxZ = (spherelist[loop].z + spherelist[loop].radius);

            // Update the leaf node’s parent if appropriate with the min/max
            if (b->parent != NULL && b->minZ < b->parent->minZ)
                b->parent->minZ = b->minZ;
            if (b->parent != NULL && b->maxZ > b->parent->maxZ)
                b->parent->maxZ = b->maxZ;

            // store our object into this nodes object list
            b->leafObjects[loop].x = spherelist[loop].x;
            b->leafObjects[loop].y = spherelist[loop].y;
            b->leafObjects[loop].z = spherelist[loop].z;
            b->leafObjects[loop].r = spherelist[loop].r;
            b->leafObjects[loop].g = spherelist[loop].g;
            b->leafObjects[loop].b = spherelist[loop].b;
            b->leafObjects[loop].idx = spherelist[loop].idx;
            b->leafObjects[loop].radius = spherelist[loop].radius;
            b->leafObjects[loop].radius2 = spherelist[loop].radius2;

            // store this leaf node back in out object so we can quickly find what leaf node our object is stored in
        }
        // done with this branch, return recursively and on return update the parent min/max bounding volume
        return;
    }

    b->nleafObjects = 0;
    Sphere *newlist = (Sphere*)malloc(count*sizeof(Sphere));

    // if we have more than one object then sort the list and create the bvhGObj
    for (loop = 0; loop < count; loop++)
    {
        // first create a new list using just the subject of objects from the old list
        newlist[loop].x = spherelist[loop].x;
        newlist[loop].y = spherelist[loop].y;
        newlist[loop].z = spherelist[loop].z;
        newlist[loop].r = spherelist[loop].r;
        newlist[loop].g = spherelist[loop].g;
        newlist[loop].b = spherelist[loop].b;
        newlist[loop].idx = spherelist[loop].idx;
        newlist[loop].radius = spherelist[loop].radius;
        newlist[loop].radius2 = spherelist[loop].radius2;
    }

    switch (axisid) // sort along the appropriate axis
    {
    case X: // X
        std::sort(newlist, newlist+count, bvhsortx);
        if (newlist[0].x == newlist[count-1].x)
            center = (size_t)(count * 0.5f);
        else
            center = std::distance(newlist, std::lower_bound(newlist, newlist+count, (double)((newlist[0].x + newlist[count-1].x) * 0.5f), bvhcompx));
        break;
    case Y: // Y
        std::sort(newlist, newlist+count, bvhsorty);
        if (newlist[0].y == newlist[count-1].y)
            center = (size_t)(count * 0.5f);
        else
            center = std::distance(newlist, std::lower_bound(newlist, newlist+count, (double)((newlist[0].y + newlist[count-1].y) * 0.5f), bvhcompy));
        break;
    case Z: // Z
        std::sort(newlist, newlist+count, bvhsortz);
        if (newlist[0].z == newlist[count-1].z)
            center = (size_t)(count * 0.5f);
        else
            center = std::distance(newlist, std::lower_bound(newlist, newlist+count, (double)((newlist[0].z + newlist[count-1].z) * 0.5f), bvhcompz));
        break;
    }

    // Find the center object in our current sub-list
    //center = (size_t)(count * 0.5f);

    // Initialize the branch to a starting value, then we’ll update it based on the leaf node recursion updating the parent
    b->minX = newlist[0].x - newlist[0].radius;
    b->maxX = newlist[0].x + newlist[0].radius;
    b->minY = newlist[0].y - newlist[0].radius;
    b->maxY = newlist[0].y + newlist[0].radius;
    b->minZ = newlist[0].z - newlist[0].radius;
    b->maxZ = newlist[0].z + newlist[0].radius;
    b->leafObjects = NULL;

    // if we’re here then we’re still in a leaf node. therefore we need to split prev/next and keep branching until we reach the leaf node
    BVH *temp = (BVH*)nodeData->mem_malloc(nodeData, sizeof(BVH));
    buildBVH(temp, newlist, b,center, NextAxis(axisid), nodeData, objData); // Split the Hierarchy to the left
    b->prev = temp;

    BVH *temp1 = (BVH*)nodeData->mem_malloc(nodeData, sizeof(BVH));
    buildBVH(temp1, newlist+center, b, count-center, NextAxis(axisid), nodeData, objData); // Split the Hierarchy to the Right
    b->next = temp1;

    free(newlist);

    // Update the parent bounding box to ensure it includes the children. Note: the leaf node already updated it’s parent, but now that parent needs to keep updating it’s branch parent until we reach the root level
    if (b->parent != NULL && b->minX < b->parent->minX)
        b->parent->minX =b-> minX;
    if (b->parent != NULL && b->maxX > b->parent->maxX)
        b->parent->maxX = b->maxX;
    if (b->parent != NULL && b->minY < b->parent->minY)
        b->parent->minY = b->minY;
    if (b->parent != NULL && b->maxY > b->parent->maxY)
        b->parent->maxY = b->maxY;
    if (b->parent != NULL && b->minZ < b->parent->minZ)
        b->parent->minZ = b->minZ;
    if (b->parent != NULL && b->maxZ > b->parent->maxZ)
        b->parent->maxZ = b->maxZ;

    return;
}

static bool doesPointLieInsideBVH(BVH* node, double x, double y, double z)
{
    bool retVal = false;
    if ((x >= node->minX) && (y>= node->minY) /*&& (z>= node->minZ)*/ && (x <= node->maxX) && (y<= node->maxY) /* && (z <= node->maxZ)*/)
        retVal = true;
    return retVal;
}

static bool processLeaf(OutIdx *v, Sphere *LeafObjects, size_t n, double x, double y, double z)
{
    for (size_t i = 0; i < n; i++)
    {
        Sphere s = LeafObjects[i];
        if (containsPoint(&s, x, y, z))
        {
#ifdef RAYTRACE
            v[s.idx].idx = s.idx;
#else
            v->idx = s.idx;
#endif
        }
    }
    return true;
}

void traverseBVHRecursive(BVH *b, OutIdx *list, size_t *start, double x, double y, double z)
{
    // Bounding box overlaps the query => process node.
    if(doesPointLieInsideBVH(b,x,y,z))    
    {
        // Leaf node => report collision.
        if (b->nleafObjects!=0)
        {
            for(int k=0;k<b->nleafObjects;k++)
            {
                list[*start].idx=b->leafObjects[k].idx;
                *start+=1;
            }
        }
        // Internal node => recurse to children.
        else
        {
            if(b->prev)
                traverseBVHRecursive(b->prev,list,start,x,y,z);
            if(b->next)
                traverseBVHRecursive(b->next,list,start,x,y,z);
        }
    }
}

int computeResult()
{
	 /* ***************************** */
	/* COMPUTE 
	/* ***************************** */
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
void traverseBVHIterative(BVH *b, OutIdx *list, size_t *start, double x, double y, double z)
{
    // Allocate traversal stack from thread-local memory,
    // and push NULL to indicate that there are no postponed nodes.
    //BVH *stack[64]={NULL};
    //BVH **stackPtr =stack;
    //*stackPtr++=NULL;

    // Traverse nodes starting from the root.
    BVH* node = b;
    do
    {
        // Check each child node for overlap.
        BVH* childL = node->prev;
        BVH* childR = node->next;
        bool overlapL = doesPointLieInsideBVH(node->prev,x,y,z);
        bool overlapR = doesPointLieInsideBVH(node->next,x,y,z);

        // Query overlaps a leaf node => report collision.
        if (overlapL && childL->nleafObjects!=0)
#ifdef RAYTRACE
            for(int k=0;k<childL->nleafObjects;k++)
            {
                list[*start].idx=childL->leafObjects[k].idx;
                *start+=1;
            }
#else
            //processLeaf(list, childL->leafObjects, childL->nleafObjects, x, y, z);
			list[*start].idx=childL->leafObjects[0].idx;//+computeResult();
#endif
        if (overlapR && childR->nleafObjects!=0)
#ifdef RAYTRACE
            for(int k=0;k<childR->nleafObjects;k++)
            {
                list[*start].idx=childR->leafObjects[k].idx;
                *start+=1;
            }
#else
            //processLeaf(list, childR->leafObjects, childR->nleafObjects, x, y, z);
			list[*start].idx=childR->leafObjects[0].idx;//+computeResult();
#endif
        // Query overlaps an internal node => traverse.
        bool traverseL = (overlapL && !node->prev->nleafObjects);
        bool traverseR = (overlapR && !node->next->nleafObjects);
#if 0
        if (!traverseL && !traverseR)
        {
            node = *--stackPtr; // pop
        }
        else
        {
            node = (traverseL) ? childL : childR;
            if (traverseL && traverseR)
                *stackPtr++ = childR; // push
        }
#endif
		node=NULL;
		if (traverseL || traverseR)
		 node = (traverseL) ? childL : childR;
    }
	while(node);
}

void destroyBVH(BVH *b)
{
    if((b->prev == NULL) && (b->next == NULL))
    {
        //leaf Node
        b->leafObjects = NULL;
    }
    else
    {
        //not a leaf node
        if(b->prev != NULL)
            destroyBVH(b->prev);

        if(b->next != NULL)
            destroyBVH(b->next);
        b->prev = NULL;
        b->next = NULL;
    }
    return;
}
