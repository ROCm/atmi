#include "atmi.h"

#define PLASMA_enum	int

typedef union GPtr {
    __global float *f;
    __global float2 *f2v;
    __global float4 *f4v;
    __global float8 *f8v;
    __global float16 *f16v;
} GPtr;

typedef union LPtr {
    __local float *f;
    __local float2 *f2v;
    __local float4 *f4v;
    __local float8 *f8v;
    __local float16 *f16v;
} LPtr;

typedef union PPtr {
    float *f;
    float2 *f2v;
    float4 *f4v;
    float8 *f8v;
    float16 *f16v;
} PPtr;


struct gemm_debug_s {
    uint M;
    uint N;
    uint K;
    float alpha;
    float A;
    float beta;
    float B;
    float C;
    uint LDA;
    uint LDB;
    uint LDC;
};


__kernel void
NNNatmi_sgemm_kernel_gpu(
    __global atmi_task_t *thisTask, __global uint *debug_info1,
    int m, int n, int k,
    PLASMA_enum transA, PLASMA_enum transB,
    uint M, uint N, uint K,
    float alpha, const __global float *A_tmp, uint lda,
    const __global float *B_tmp, uint ldb,
    float beta, __global float *C_tmp, uint ldc, __global struct gemm_debug_s *debug_info2)
{
    *debug_info1 = 222;
    debug_info2->M = M;
    debug_info2->N = N;
    debug_info2->K = K;
    debug_info2->alpha = alpha;
    debug_info2->A = A_tmp[0];
    debug_info2->beta = beta;
    debug_info2->B = B_tmp[0];
    debug_info2->C = C_tmp[0];
    debug_info2->LDA = lda;
    debug_info2->LDB = ldb;
    debug_info2->LDC = ldc;
}

__attribute__((reqd_work_group_size(8, 8, 1)))
__kernel void
atmi_sgemm_kernel_gpu(
    __global atmi_task_t *thisTask, __global uint *debug_info1,
    int m, int n, int k,
    PLASMA_enum transA, PLASMA_enum transB,
    uint M, uint N, uint K,
    float alpha, const __global float *A_tmp, uint lda, 
    const __global float *B_tmp, uint ldb, 
    float beta, __global float *C_tmp, uint ldc, __global struct gemm_debug_s *debug_info2)
{
    debug_info2->M = 222;
    float8* A = (float8 *)A_tmp;
    float8* B = (float8 *)B_tmp;
    float8* C = (float8 *)C_tmp;
    float8 a0, a1, a2, a3;
    float8 b0, b1, b2, b3;
    float8 c0, c1, c2, c3, c4, c5, c6, c7;
    uint4 coord = 0u; /* contains coordB, coordA, k */

    uint offA = 0;
    uint offB = 0;
    uint offC = 0;

    lda /= 8;
    ldb /= 8;
    uint kif;
    uint get_group_id_1;
    uint get_global_id_1;
    A += (uint)get_global_id(0);
    get_group_id_1 = (get_group_id(0) + get_group_id(1))% get_num_groups(1);
    get_global_id_1 = get_group_id_1 * get_local_size(1) + get_local_id(1);
    kif = (N % 512 != 0);
    get_global_id_1 = (kif*(uint)get_global_id(1)) + ((1-kif)*get_global_id_1);
    B += get_global_id_1;
    coord.y = 8u * (uint)get_global_id(0);
    coord.x = 8u * (uint)get_global_id_1;
    A += offA / 8;
    B += offB / 8;
    C += offC / 8;
    c0 = 0;
    c1 = 0;
    c2 = 0;
    c3 = 0;
    c4 = 0;
    c5 = 0;
    c6 = 0;
    c7 = 0;

    for (uint k1 = 0; k1 < K; k1 += 4) {
        /* -- Tiles multiplier -- */
        b0 = B[0];
        b1 = B[ldb];
        b2 = B[(ldb << 1)];
        b3 = B[mad24(3u, ldb, 0u)];

        a0 = A[0];
        a1 = A[lda];
        a2 = A[(lda << 1)];
        a3 = A[mad24(3u, lda, 0u)];

        c0 = mad(a0, b0.s0, c0);
        c1 = mad(a0, b0.s1, c1);
        c2 = mad(a0, b0.s2, c2);
        c3 = mad(a0, b0.s3, c3);
        c4 = mad(a0, b0.s4, c4);
        c5 = mad(a0, b0.s5, c5);
        c6 = mad(a0, b0.s6, c6);
        c7 = mad(a0, b0.s7, c7);

        c0 = mad(a1, b1.s0, c0);
        c1 = mad(a1, b1.s1, c1);
        c2 = mad(a1, b1.s2, c2);
        c3 = mad(a1, b1.s3, c3);
        c4 = mad(a1, b1.s4, c4);
        c5 = mad(a1, b1.s5, c5);
        c6 = mad(a1, b1.s6, c6);
        c7 = mad(a1, b1.s7, c7);

        c0 = mad(a2, b2.s0, c0);
        c1 = mad(a2, b2.s1, c1);
        c2 = mad(a2, b2.s2, c2);
        c3 = mad(a2, b2.s3, c3);
        c4 = mad(a2, b2.s4, c4);
        c5 = mad(a2, b2.s5, c5);
        c6 = mad(a2, b2.s6, c6);
        c7 = mad(a2, b2.s7, c7);

        c0 = mad(a3, b3.s0, c0);
        c1 = mad(a3, b3.s1, c1);
        c2 = mad(a3, b3.s2, c2);
        c3 = mad(a3, b3.s3, c3);
        c4 = mad(a3, b3.s4, c4);
        c5 = mad(a3, b3.s5, c5);
        c6 = mad(a3, b3.s6, c6);
        c7 = mad(a3, b3.s7, c7);

        A += (lda << 2);
        B += (ldb << 2);
        /* ---------------------- */
    }


    GPtr uC;

    uC.f = C + (coord.x * ldc + coord.y)/8;

    __global float8 *pC = uC.f8v;

    float8 tempC0, tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7;

    tempC0 = pC[0];
    tempC1 = pC[(ldc >> 3)];
    tempC2 = pC[(ldc >> 2)];
    tempC3 = pC[mad24(3u, (ldc >> 3), 0u)];
    tempC4 = pC[(ldc >> 1)];
    tempC5 = pC[mad24(5u, (ldc >> 3), 0u)];
    tempC6 = pC[mad24(6u, (ldc >> 3), 0u)];
    tempC7 = pC[mad24(7u, (ldc >> 3), 0u)];
    tempC0 = mad(tempC0, beta, 0);
    tempC1 = mad(tempC1, beta, 0);
    tempC2 = mad(tempC2, beta, 0);
    tempC3 = mad(tempC3, beta, 0);
    tempC4 = mad(tempC4, beta, 0);
    tempC5 = mad(tempC5, beta, 0);
    tempC6 = mad(tempC6, beta, 0);
    tempC7 = mad(tempC7, beta, 0);
    tempC0 = mad(c0, alpha, tempC0);
    tempC1 = mad(c1, alpha, tempC1);
    tempC2 = mad(c2, alpha, tempC2);
    tempC3 = mad(c3, alpha, tempC3);
    tempC4 = mad(c4, alpha, tempC4);
    tempC5 = mad(c5, alpha, tempC5);
    tempC6 = mad(c6, alpha, tempC6);
    tempC7 = mad(c7, alpha, tempC7);
    pC[0] = tempC0;
    pC[(ldc >> 3)] = tempC1;
    pC[(ldc >> 2)] = tempC2;
    pC[mad24(3u, (ldc >> 3), 0u)] = tempC3;
    pC[(ldc >> 1)] = tempC4;
    pC[mad24(5u, (ldc >> 3), 0u)] = tempC5;
    pC[mad24(6u, (ldc >> 3), 0u)] = tempC6;
    pC[mad24(7u, (ldc >> 3), 0u)] = tempC7;

}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void
atmi_ssyrk_kernel_gpu(
    __global atmi_task_t *thisTask,
    int m, int k, PLASMA_enum uplo, PLASMA_enum trans,
    uint N,
    const uint K,
    const float alpha,
    const __global float/*8*/ * A_tmp,
    uint lda,
    const float beta,
    __global float *C,
    uint ldc)
{
    __global float8* A = A_tmp;
    float8 a0, a1, a2, a3;
    float8 b0, b1, b2, b3;
    float8 c0, c1, c2, c3, c4, c5, c6, c7;
    uint argN = N;
    __global float8 *B;
    uint4 coord = 0;
    uint k0 = 0;

    uint offA = 0;
    uint offB = 0;
    uint offC = 0;
    uint startN = 0;
    uint origN = 0;

    const int lid = get_local_id(0);
    uint block = get_group_id(0);

    k0 = (N + 63) / 64;
    if (block < k0 * (startN / 64)) {
        coord.x = (block / k0) * 64;
        block %= k0;
    }
    else {
        block -= k0 * (startN / 64);
        while (block >= k0) {
            block -= k0;
            coord.x += 64;
            k0 = ((N + 7) / 8 * 8 - coord.x + 63) / 64;
        }
        coord.x += startN;
    }
    coord.y = (N + 7) / 8 * 8 - (block + 1) * 64;
    coord.y = (coord.y + 7) / 8 * 8;
    coord.y += startN + lid % 8 * 8;
    coord.x += lid / 8 * 8;
    if (coord.y >= startN + N || coord.x >= startN + N) {
        return;
    }
    if (coord.x >= coord.y + 8) {
        return;
    }


    c0 = 0;
    c1 = 0;
    c2 = 0;
    c3 = 0;
    c4 = 0;
    c5 = 0;
    c6 = 0;
    c7 = 0;

    // Set N to initial argument of blas function, not divided one
    N = origN;
    lda /= 8;

    B = A + (coord.x >> 3) + (offA >> 3);
    A = A + (coord.y >> 3) + (offA >> 3);
    C = C + offC;
    for (k0 = 0; k0 < K; k0 += 4) {
        /* -- Tiles multiplier -- */
        b0 = B[0];
        b1 = B[lda];
        b2 = B[(lda << 1)];
        b3 = B[mad24(3u, lda, 0u)];

        a0 = A[0];
        a1 = A[lda];
        a2 = A[(lda << 1)];
        a3 = A[mad24(3u, lda, 0u)];

        c0 = mad(a0, b0.s0, c0);
        c1 = mad(a0, b0.s1, c1);
        c2 = mad(a0, b0.s2, c2);
        c3 = mad(a0, b0.s3, c3);
        c4 = mad(a0, b0.s4, c4);
        c5 = mad(a0, b0.s5, c5);
        c6 = mad(a0, b0.s6, c6);
        c7 = mad(a0, b0.s7, c7);

        c0 = mad(a1, b1.s0, c0);
        c1 = mad(a1, b1.s1, c1);
        c2 = mad(a1, b1.s2, c2);
        c3 = mad(a1, b1.s3, c3);
        c4 = mad(a1, b1.s4, c4);
        c5 = mad(a1, b1.s5, c5);
        c6 = mad(a1, b1.s6, c6);
        c7 = mad(a1, b1.s7, c7);

        c0 = mad(a2, b2.s0, c0);
        c1 = mad(a2, b2.s1, c1);
        c2 = mad(a2, b2.s2, c2);
        c3 = mad(a2, b2.s3, c3);
        c4 = mad(a2, b2.s4, c4);
        c5 = mad(a2, b2.s5, c5);
        c6 = mad(a2, b2.s6, c6);
        c7 = mad(a2, b2.s7, c7);

        c0 = mad(a3, b3.s0, c0);
        c1 = mad(a3, b3.s1, c1);
        c2 = mad(a3, b3.s2, c2);
        c3 = mad(a3, b3.s3, c3);
        c4 = mad(a3, b3.s4, c4);
        c5 = mad(a3, b3.s5, c5);
        c6 = mad(a3, b3.s6, c6);
        c7 = mad(a3, b3.s7, c7);

        A += (lda << 2);
        B += (lda << 2);
        /* ---------------------- */
    }


    if ( !( (coord.y >= startN + argN) || (coord.x >= startN + argN) || (coord.x >= coord.y + 8) ) ) {
        if (coord.x + 8 > coord.y) {
            __global float *dst = C + coord.x * ldc + coord.y;

            float8 tempC0, tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7;




            tempC0 = *(__global float8*)(&dst[0]);
            tempC1.s1 = *(__global float*)(&dst[ldc + 1]);
            tempC1.s23 = *(__global float2*)(&dst[ldc + 2]);
            tempC1.s4567 = *(__global float4*)(&dst[ldc + 4]);
            tempC2.s23 = *(__global float2*)(&dst[mad24(2u, ldc, 2u)]);
            tempC2.s4567 = *(__global float4*)(&dst[mad24(2u, ldc, 4u)]);
            tempC3.s3 = *(__global float*)(&dst[mad24(3u, ldc, 3u)]);
            tempC3.s4567 = *(__global float4*)(&dst[mad24(3u, ldc, 4u)]);
            tempC4.s4567 = *(__global float4*)(&dst[mad24(4u, ldc, 4u)]);
            tempC5.s5 = *(__global float*)(&dst[mad24(5u, ldc, 5u)]);
            tempC5.s67 = *(__global float2*)(&dst[mad24(5u, ldc, 6u)]);
            tempC6.s67 = *(__global float2*)(&dst[mad24(6u, ldc, 6u)]);
            tempC7.s7 = *(__global float*)(&dst[mad24(7u, ldc, 7u)]);

            tempC0 = mad(tempC0, beta, 0);
            tempC0 = mad(c0, alpha, tempC0);
            tempC1.s1 = mad(tempC1.s1, beta, 0);
            tempC1.s1 = mad(c1.s1, alpha, tempC1.s1);
            tempC1.s23 = mad(tempC1.s23, beta, 0);
            tempC1.s23 = mad(c1.s23, alpha, tempC1.s23);
            tempC1.s4567 = mad(tempC1.s4567, beta, 0);
            tempC1.s4567 = mad(c1.s4567, alpha, tempC1.s4567);
            tempC2.s23 = mad(tempC2.s23, beta, 0);
            tempC2.s23 = mad(c2.s23, alpha, tempC2.s23);
            tempC2.s4567 = mad(tempC2.s4567, beta, 0);
            tempC2.s4567 = mad(c2.s4567, alpha, tempC2.s4567);
            tempC3.s3 = mad(tempC3.s3, beta, 0);
            tempC3.s3 = mad(c3.s3, alpha, tempC3.s3);
            tempC3.s4567 = mad(tempC3.s4567, beta, 0);
            tempC3.s4567 = mad(c3.s4567, alpha, tempC3.s4567);
            tempC4.s4567 = mad(tempC4.s4567, beta, 0);
            tempC4.s4567 = mad(c4.s4567, alpha, tempC4.s4567);
            tempC5.s5 = mad(tempC5.s5, beta, 0);
            tempC5.s5 = mad(c5.s5, alpha, tempC5.s5);
            tempC5.s67 = mad(tempC5.s67, beta, 0);
            tempC5.s67 = mad(c5.s67, alpha, tempC5.s67);
            tempC6.s67 = mad(tempC6.s67, beta, 0);
            tempC6.s67 = mad(c6.s67, alpha, tempC6.s67);
            tempC7.s7 = mad(tempC7.s7, beta, 0);
            tempC7.s7 = mad(c7.s7, alpha, tempC7.s7);

            *(__global float8*)(&dst[0]) = tempC0;
            *(__global float*)(&dst[ldc + 1]) = tempC1.s1;
            *(__global float2*)(&dst[ldc + 2]) = tempC1.s23;
            *(__global float4*)(&dst[ldc + 4]) = tempC1.s4567;
            *(__global float2*)(&dst[mad24(2u, ldc, 2u)]) = tempC2.s23;
            *(__global float4*)(&dst[mad24(2u, ldc, 4u)]) = tempC2.s4567;
            *(__global float*)(&dst[mad24(3u, ldc, 3u)]) = tempC3.s3;
            *(__global float4*)(&dst[mad24(3u, ldc, 4u)]) = tempC3.s4567;
            *(__global float4*)(&dst[mad24(4u, ldc, 4u)]) = tempC4.s4567;
            *(__global float*)(&dst[mad24(5u, ldc, 5u)]) = tempC5.s5;
            *(__global float2*)(&dst[mad24(5u, ldc, 6u)]) = tempC5.s67;
            *(__global float2*)(&dst[mad24(6u, ldc, 6u)]) = tempC6.s67;
            *(__global float*)(&dst[mad24(7u, ldc, 7u)]) = tempC7.s7;

        }
        else {

            GPtr uC;

            uC.f = C + coord.x * ldc + coord.y;

            __global float8 *pC = uC.f;

            float8 tempC0, tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7;

            tempC0 = pC[0];
            tempC1 = pC[(ldc >> 3)];
            tempC2 = pC[(ldc >> 2)];
            tempC3 = pC[mad24(3u, (ldc >> 3), 0u)];
            tempC4 = pC[(ldc >> 1)];
            tempC5 = pC[mad24(5u, (ldc >> 3), 0u)];
            tempC6 = pC[mad24(6u, (ldc >> 3), 0u)];
            tempC7 = pC[mad24(7u, (ldc >> 3), 0u)];
            tempC0 = mad(tempC0, beta, 0);
            tempC1 = mad(tempC1, beta, 0);
            tempC2 = mad(tempC2, beta, 0);
            tempC3 = mad(tempC3, beta, 0);
            tempC4 = mad(tempC4, beta, 0);
            tempC5 = mad(tempC5, beta, 0);
            tempC6 = mad(tempC6, beta, 0);
            tempC7 = mad(tempC7, beta, 0);
            tempC0 = mad(c0, alpha, tempC0);
            tempC1 = mad(c1, alpha, tempC1);
            tempC2 = mad(c2, alpha, tempC2);
            tempC3 = mad(c3, alpha, tempC3);
            tempC4 = mad(c4, alpha, tempC4);
            tempC5 = mad(c5, alpha, tempC5);
            tempC6 = mad(c6, alpha, tempC6);
            tempC7 = mad(c7, alpha, tempC7);
            pC[0] = tempC0;
            pC[(ldc >> 3)] = tempC1;
            pC[(ldc >> 2)] = tempC2;
            pC[mad24(3u, (ldc >> 3), 0u)] = tempC3;
            pC[(ldc >> 1)] = tempC4;
            pC[mad24(5u, (ldc >> 3), 0u)] = tempC5;
            pC[mad24(6u, (ldc >> 3), 0u)] = tempC6;
            pC[mad24(7u, (ldc >> 3), 0u)] = tempC7;
        }
    }
}



#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable

#define __ROUNDING_MODE__ rte


void
fcopyDBlockGL832(
    LPtr dst,
    GPtr src,
    uint startRow,
    uint startCol,
    uint ld)
{
    const int lid = get_local_id(0);

    src.f += (startRow + lid / 8) * ld + startCol + lid % 8 * 4;

    dst.f += (lid / 8) * 32 + (lid % 8 * 4) * 1;

    *dst.f4v++ = *src.f4v++;
}

__kernel void
nnnatmi_strsm_kernel_gpu(
    __global atmi_task_t *thisTask,
    __global uint *debug_info1,
    int m,
    int k,
    PLASMA_enum side, PLASMA_enum uplo, PLASMA_enum transA, PLASMA_enum diag,
    uint N,
    uint M,
    float alpha,
    const __global float * A,
    uint lda,
    __global float * B,
    uint ldb, __global struct gemm_debug_s *debug_info2)
{
    *debug_info1 = 1111;
    debug_info2->M = 2222;
    debug_info2->N = N;
    debug_info2->alpha = alpha;
    debug_info2->A = A[0];
    debug_info2->LDA = lda;
    debug_info2->B = B[0];
    debug_info2->LDB = ldb;
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void
atmi_strsm_kernel_gpu(
    __global atmi_task_t *thisTask,
    __global uint *debug_info1,
    int m,
    int k,
    PLASMA_enum side, PLASMA_enum uplo, PLASMA_enum transA, PLASMA_enum diag,
    uint N,
    uint M,
    float alpha,
    const __global float * A,
    uint lda,
    __global float * B,
    uint ldb, __global struct gemm_debug_s *debug_info2)
{
    const int lid = get_local_id(0);
    const int gid = get_group_id(0);
    GPtr uA, uB;
    uint coordA, coordB;
    uint m0 = 0, k0, m1;
    float4 a0, a1, a2, a3;
    float4 b0, b1, b2, b3;
    float4 c0, c1, c2, c3;
    __local float4 tmpB[64];
    LPtr lB;
    LPtr lBMain = {(__local float*)(tmpB + lid % 8 * 1)};

    uint offA = 0;
    uint offB = 0;

    A += offA;
    B += offB;

    uB.f = B;

    for (m0 = 0; m0 < M; m0 += 32) {
        c0 = 0;
        c1 = 0;
        c2 = 0;
        c3 = 0;

        coordA = m0 + (lid / 8 * 4);
        k0 = 0;
        coordB = gid * 32 + (lid % 8 * 4);

        // Stage 1. Multiply and update with large blocks
        uA.f = A + k0 * lda;
        for (k0 = 0; k0 < m0; k0 += 8) {
            lB.f4v = tmpB;
            barrier(CLK_LOCAL_MEM_FENCE);
            fcopyDBlockGL832(lB, uB, k0, gid * 32, ldb);
            barrier(CLK_LOCAL_MEM_FENCE);

            lB = lBMain;

            /* -- Tiles multiplier -- */
            for (int k1 = 0; k1 < 8; k1 += 4) {
                b0 = lB.f4v[0];
                b1 = lB.f4v[8];
                b2 = lB.f4v[16];
                b3 = lB.f4v[24];

                a0 = uA.f4v[(coordA >> 2)];
                a1 = uA.f4v[(lda >> 2) + (coordA >> 2)];
                a2 = uA.f4v[(lda >> 1) + (coordA >> 2)];
                a3 = uA.f4v[mad24(3u, (lda >> 2), (coordA >> 2))];

                c0 = mad(b0, a0.s0, c0);
                c1 = mad(b0, a0.s1, c1);
                c2 = mad(b0, a0.s2, c2);
                c3 = mad(b0, a0.s3, c3);

                c0 = mad(b1, a1.s0, c0);
                c1 = mad(b1, a1.s1, c1);
                c2 = mad(b1, a1.s2, c2);
                c3 = mad(b1, a1.s3, c3);

                c0 = mad(b2, a2.s0, c0);
                c1 = mad(b2, a2.s1, c1);
                c2 = mad(b2, a2.s2, c2);
                c3 = mad(b2, a2.s3, c3);

                c0 = mad(b3, a3.s0, c0);
                c1 = mad(b3, a3.s1, c1);
                c2 = mad(b3, a3.s2, c2);
                c3 = mad(b3, a3.s3, c3);

                uA.f4v += lda;
                lB.f4v += 32;
            }
            /* ---------------------- */
        }
        uA.f = A;


        /*
         * Stage 2. A part of work items multiply got result on a respective
         * inverted diagonal block, and the remaining ones wait. Then they perform
         * one step of further intermediate result evaluation as multiplying tile by tile.
         * It continues until the whole panel of the matrix A is processed
         */
        for (m1 = 0; m1 < 8; m1++) {
            coordA = m0 + (lid / 8 * 4);
            k0 = m0 + m1 * 4;
            coordB = gid * 32 + (lid % 8 * 4);

            if (lid / 8 == m1) {
                 {
                    float beta = -1. / alpha;
                    float alpha = beta;
                    GPtr uC;

                    uC.f = B + coordA * ldb + coordB;

                    __global float4 *pC = uC.f;

                    float4 tempC0, tempC1, tempC2, tempC3;

                    tempC0 = pC[0];
                    tempC1 = pC[(ldb >> 2)];
                    tempC2 = pC[(ldb >> 1)];
                    tempC3 = pC[mad24(3u, (ldb >> 2), 0u)];
                    c0 = mad(c0, alpha, tempC0);
                    c1 = mad(c1, alpha, tempC1);
                    c2 = mad(c2, alpha, tempC2);
                    c3 = mad(c3, alpha, tempC3);
                }

                // Fetch and invert the square tile located on the diagonal
                b0 = uA.f4v[mad24(k0, (lda >> 2), (coordA >> 2))];
                b1 = uA.f4v[mad24(k0 + 1, (lda >> 2), (coordA >> 2))];
                b2 = uA.f4v[mad24(k0 + 2, (lda >> 2), (coordA >> 2))];
                b3 = uA.f4v[mad24(k0 + 3, (lda >> 2), (coordA >> 2))];

                // post fetch A
                {
                    uint zy = k0;
                    b0.s0 = zy > coordA ? 0 : b0.s0;
                    b0.s1 = zy > coordA + 1 ? 0 : b0.s1;
                    b0.s2 = zy > coordA + 2 ? 0 : b0.s2;
                    b0.s3 = zy > coordA + 3 ? 0 : b0.s3;
                    zy++;
                    b1.s0 = zy > coordA ? 0 : b1.s0;
                    b1.s1 = zy > coordA + 1 ? 0 : b1.s1;
                    b1.s2 = zy > coordA + 2 ? 0 : b1.s2;
                    b1.s3 = zy > coordA + 3 ? 0 : b1.s3;
                    zy++;
                    b2.s0 = zy > coordA ? 0 : b2.s0;
                    b2.s1 = zy > coordA + 1 ? 0 : b2.s1;
                    b2.s2 = zy > coordA + 2 ? 0 : b2.s2;
                    b2.s3 = zy > coordA + 3 ? 0 : b2.s3;
                    zy++;
                    b3.s0 = zy > coordA ? 0 : b3.s0;
                    b3.s1 = zy > coordA + 1 ? 0 : b3.s1;
                    b3.s2 = zy > coordA + 2 ? 0 : b3.s2;
                    b3.s3 = zy > coordA + 3 ? 0 : b3.s3;
                }
                // Invert tile
                a0 = 0;
                a1 = 0;
                a2 = 0;
                a3 = 0;

                a0.s0 = 1;
                a1.s1 = 1;
                a2.s2 = 1;
                a3.s3 = 1;

                a0.s0 /= b0.s0;
                a1.s0 /= b0.s0;
                a2.s0 /= b0.s0;
                a3.s0 /= b0.s0;

                a0.s1 -= a0.s0 * b0.s1;
                a0.s1 /= b1.s1;
                a1.s1 -= a1.s0 * b0.s1;
                a1.s1 /= b1.s1;
                a2.s1 -= a2.s0 * b0.s1;
                a2.s1 /= b1.s1;
                a3.s1 -= a3.s0 * b0.s1;
                a3.s1 /= b1.s1;
                a0.s2 -= a0.s0 * b0.s2;
                a1.s2 -= a1.s0 * b0.s2;
                a2.s2 -= a2.s0 * b0.s2;
                a3.s2 -= a3.s0 * b0.s2;
                a0.s3 -= a0.s0 * b0.s3;
                a1.s3 -= a1.s0 * b0.s3;
                a2.s3 -= a2.s0 * b0.s3;
                a3.s3 -= a3.s0 * b0.s3;

                a0.s2 -= a0.s1 * b1.s2;
                a0.s2 /= b2.s2;
                a1.s2 -= a1.s1 * b1.s2;
                a1.s2 /= b2.s2;
                a2.s2 -= a2.s1 * b1.s2;
                a2.s2 /= b2.s2;
                a3.s2 -= a3.s1 * b1.s2;
                a3.s2 /= b2.s2;
                a0.s3 -= a0.s1 * b1.s3;
                a1.s3 -= a1.s1 * b1.s3;
                a2.s3 -= a2.s1 * b1.s3;
                a3.s3 -= a3.s1 * b1.s3;

                a0.s3 -= a0.s2 * b2.s3;
                a0.s3 /= b3.s3;
                a1.s3 -= a1.s2 * b2.s3;
                a1.s3 /= b3.s3;
                a2.s3 -= a2.s2 * b2.s3;
                a2.s3 /= b3.s3;
                a3.s3 -= a3.s2 * b2.s3;
                a3.s3 /= b3.s3;

                b0 = c0;
                b1 = c1;
                b2 = c2;
                b3 = c3;

                c0 = 0;
                c1 = 0;
                c2 = 0;
                c3 = 0;

                c0 = mad(b0, a0.s0, c0);
                c1 = mad(b0, a0.s1, c1);
                c2 = mad(b0, a0.s2, c2);
                c3 = mad(b0, a0.s3, c3);
                c0 = mad(b1, a1.s0, c0);
                c1 = mad(b1, a1.s1, c1);
                c2 = mad(b1, a1.s2, c2);
                c3 = mad(b1, a1.s3, c3);
                c0 = mad(b2, a2.s0, c0);
                c1 = mad(b2, a2.s1, c1);
                c2 = mad(b2, a2.s2, c2);
                c3 = mad(b2, a2.s3, c3);
                c0 = mad(b3, a3.s0, c0);
                c1 = mad(b3, a3.s1, c1);
                c2 = mad(b3, a3.s2, c2);
                c3 = mad(b3, a3.s3, c3);

                // Write back the given result

                GPtr uC;

                uC.f = B + coordA * ldb + coordB;

                __global float4 *pC = uC.f;

                float4 tempC0, tempC1, tempC2, tempC3;

                tempC0 = mad(c0, alpha, 0);
                tempC1 = mad(c1, alpha, 0);
                tempC2 = mad(c2, alpha, 0);
                tempC3 = mad(c3, alpha, 0);
                pC[0] = tempC0;
                pC[(ldb >> 2)] = tempC1;
                pC[(ldb >> 1)] = tempC2;
                pC[mad24(3u, (ldb >> 2), 0u)] = tempC3;
            }
            barrier(CLK_GLOBAL_MEM_FENCE);

            if (lid / 8 > m1) {
                /* -- Tiles multiplier -- */
                b0 = uB.f4v[mad24(k0, (ldb >> 2), (coordB >> 2) % (ldb >> 2))];
                b1 = uB.f4v[mad24(k0 + 1, (ldb >> 2), (coordB >> 2) % (ldb >> 2))];
                b2 = uB.f4v[mad24(k0 + 2, (ldb >> 2), (coordB >> 2) % (ldb >> 2))];
                b3 = uB.f4v[mad24(k0 + 3, (ldb >> 2), (coordB >> 2) % (ldb >> 2))];

                a0 = uA.f4v[mad24(k0, (lda >> 2), (coordA >> 2))];
                a1 = uA.f4v[mad24(k0 + 1, (lda >> 2), (coordA >> 2))];
                a2 = uA.f4v[mad24(k0 + 2, (lda >> 2), (coordA >> 2))];
                a3 = uA.f4v[mad24(k0 + 3, (lda >> 2), (coordA >> 2))];

                c0 = mad(b0, a0.s0, c0);
                c1 = mad(b0, a0.s1, c1);
                c2 = mad(b0, a0.s2, c2);
                c3 = mad(b0, a0.s3, c3);

                c0 = mad(b1, a1.s0, c0);
                c1 = mad(b1, a1.s1, c1);
                c2 = mad(b1, a1.s2, c2);
                c3 = mad(b1, a1.s3, c3);

                c0 = mad(b2, a2.s0, c0);
                c1 = mad(b2, a2.s1, c1);
                c2 = mad(b2, a2.s2, c2);
                c3 = mad(b2, a2.s3, c3);

                c0 = mad(b3, a3.s0, c0);
                c1 = mad(b3, a3.s1, c1);
                c2 = mad(b3, a3.s2, c2);
                c3 = mad(b3, a3.s3, c3);
                /* ---------------------- */
            }
            barrier(CLK_GLOBAL_MEM_FENCE);
        }
    }
}

__kernel void
//doPotrf(float* dA,  magma_int_t ldda, int invocationID, magma_int_t n) {}
doPotrf(__global float * dA,  int ldda, int invocationID, int n) {}

__kernel void
doNothing() {}
