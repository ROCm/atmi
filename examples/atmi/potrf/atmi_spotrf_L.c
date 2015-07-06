#include <stdio.h>

#include <core_blas.h>

#include <data_dist/matrix/sym_two_dim_rectangle_cyclic.h>

#include "atmi_lapack.h"

#include "atmi.h"

#ifdef __cplusplus
#define _CPPSTRING_ "C"
#endif
#ifndef __cplusplus
#define _CPPSTRING_
#endif

#define ATMI_BLKLDD( _desc_, _m_ ) ( (_desc_)->storage == matrix_Tile ? (_desc_)->mb : (_desc_)->llm )

extern _CPPSTRING_ void atmi_spotrf_kernel_cpu(atmi_task_t *thisTask, int k, PLASMA_enum uplo, int N, float *A, int LDA) __attribute__((atmi_task_impl("cpu", "atmi_spotrf_kernel")));
extern _CPPSTRING_ void atmi_strsm_kernel_cpu(atmi_task_t *thisTask, int m, int k, PLASMA_enum side, PLASMA_enum uplo, PLASMA_enum transA, PLASMA_enum diag, int M, int N, int alpha, float *A, int LDA, float *B, int LDB) __attribute__((atmi_task_impl("cpu", "atmi_strsm_kernel")));
extern _CPPSTRING_ void atmi_ssyrk_kernel_cpu(atmi_task_t *thisTask, int m, int k, PLASMA_enum uplo, PLASMA_enum trans, int N, int K, int alpha, const float *A, int LDA, int beta, float *C, int LDC) __attribute__((atmi_task_impl("cpu", "atmi_ssyrk_kernel")));
extern _CPPSTRING_ void atmi_sgemm_kernel_cpu(atmi_task_t *thisTask, int m, int n, int k, PLASMA_enum transA, int transB, int M, int N, int K, int alpha, const float *A, int LDA, const float *B, int LDB, int beta, float *C, int LDC) __attribute__((atmi_task_impl("cpu", "atmi_sgemm_kernel")));


void atmi_spotrf_kernel_cpu(atmi_task_t *thisTask, int k, PLASMA_enum uplo, int N, float *A, int LDA)
{
    int info = 0;
    printf("POTRF(%d)\n", k);
    CORE_spotrf(uplo, N, A, LDA, &info);
}

void atmi_strsm_kernel_cpu(atmi_task_t *thisTask, int m, int k, PLASMA_enum side, PLASMA_enum uplo, PLASMA_enum transA, PLASMA_enum diag, int M, int N, int alpha, float *A, int LDA, float *B, int LDB)
{
    printf("TRSM(%d, %d)\n", m, k);
    CORE_strsm(side, uplo, transA, diag, M, N, (float)alpha, A, LDA, B, LDB);
}

void atmi_ssyrk_kernel_cpu(atmi_task_t *thisTask, int m, int k, PLASMA_enum uplo, PLASMA_enum trans, int N, int K, int alpha, const float *A, int LDA, int beta, float *C, int LDC)
{
    printf("SYRK(%d, %d)\n", k, m);
    CORE_ssyrk(uplo, trans, N, K, (float)alpha, A, LDA, (float)beta, C, LDC);
}	 

void atmi_sgemm_kernel_cpu(atmi_task_t *thisTask, int m, int n, int k, PLASMA_enum transA, int transB, int M, int N, int K, int alpha, const float *A, int LDA, const float *B, int LDB, int beta, float *C, int LDC)
{
    printf("GEMM(%d, %d, %d)\n", m, n, k);
    CORE_sgemm(transA, transB, M, N, K, (float)alpha, A, LDA, B, LDB, (float)beta, C, LDC);
} 

void* sym_two_dim_block_cyclic_lookup_matrix(tiled_matrix_desc_t *descA, int m, int n)
{
    sym_two_dim_block_cyclic_t *Ddesc = (sym_two_dim_block_cyclic_t *)descA;
    size_t pos = sym_twoDBC_coordinates_to_position(Ddesc, m, n);
    return Ddesc->mat + pos * descA->bsiz * sizeof(float);
}

int atmi_spotrf_L(PLASMA_enum uplo, tiled_matrix_desc_t *descA)
{
    int k, m, n;
    int tempkm, tempmm;
    int ldak, ldam, ldan;
    atmi_task_t *null_task;
    float *T, *C, *A, *B;
    for(k = 0; k <= descA->mt-1; k++) {
        tempkm = (k == (descA->mt - 1)) ? descA->m - k * descA->mb : descA->mb;
        ldak = ATMI_BLKLDD(descA, k);
        T = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, k, k);
        atmi_spotrf_kernel_cpu(null_task, k, uplo, tempkm, T, ldak);
        
        for(m = k+1; m <= descA->mt-1; m++) {
            tempmm = m == descA->mt - 1 ? descA->m - m * descA->mb : descA->mb;
            ldam = ATMI_BLKLDD(descA, m);
            T = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, k, k);
            C = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, k);
            atmi_strsm_kernel_cpu(null_task, m, k, PlasmaRight, PlasmaLower, PlasmaTrans, PlasmaNonUnit, tempmm, descA->nb, 1.0F, T, ldak, C, ldam);
        }
        
        for(m = k+1; m <= descA->mt-1; m++) {
            tempmm = m == descA->mt - 1 ? descA->m - m * descA->mb : descA->mb;
            ldam = ATMI_BLKLDD(descA, m);
            T = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, m);
            A = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, k);
            atmi_ssyrk_kernel_cpu(null_task, m, k, PlasmaLower, PlasmaNoTrans, tempmm, descA->mb, -1.0F, A, ldam, 1.0F, T, ldam);
            
            for(n = k+1; n < m; n++) {
                ldan = ATMI_BLKLDD(descA, n);
                A = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, k);
                B = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, n, k);
                C = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, n);
                atmi_sgemm_kernel_cpu(null_task, m, n, k, PlasmaNoTrans, PlasmaTrans, tempmm, descA->mb, descA->mb, -1.0F, A, ldam, B, ldan, 1.0F, C, ldam);
            }
        }
        
        
    }
    return 0;
}

int atmi_task_spotrf_L(PLASMA_enum uplo, tiled_matrix_desc_t *descA)
{
    int k, m, n;
    int tempkm, tempmm;
    int ldak, ldam, ldan;
    float *T, *C, *A, *B;
    
    ATMI_LPARM_CPU(cpu_lp);
//    cpu_lp->synchronous = ATMI_TRUE;

    atmi_task_t *spotrf_tasks[descA->mt]; /* there are descA->mt -1 POTRF tasks */
//    atmi_task_t *strsm_task[descA->mt*(descA->mt-1)/2];  /* descA->mt-1 + descA->mt-2 + ... + 1 */
//    atmi_task_t *ssyrk_task[descA->mt*(descA->mt-1)/2];  /* same as TRSM */
    atmi_task_t *strsm_tasks[descA->mt][descA->mt-1]; /* mt-1, mt-2 */
    atmi_task_t *ssyrk_tasks[descA->mt-1][descA->mt]; /* mt-2, mt-1 */
    atmi_task_t *sgemm_tasks[descA->mt][descA->mt-1][descA->mt-2]; /* mt-1, mt-2, mt-3 */
    
    atmi_task_t *required_tasks[3];
    
    for(k = 0; k <= descA->mt-1; k++){
        tempkm = (k == (descA->mt - 1)) ? descA->m - k * descA->mb : descA->mb;
        ldak = ATMI_BLKLDD(descA, k);
        float *T = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, k, k);
        if (k > 0) { 
            /* set dependency for potrf except the first one */
            cpu_lp->num_required = 1;
            required_tasks[0] = ssyrk_tasks[k-1][k];
            cpu_lp->requires = required_tasks;
        }
        spotrf_tasks[k] = atmi_spotrf_kernel(cpu_lp, k, uplo, tempkm, T, ldak);
        
        for(m = k+1; m <= descA->mt-1; m++) {
            tempmm = m == descA->mt - 1 ? descA->m - m * descA->mb : descA->mb;
            ldam = ATMI_BLKLDD(descA, m);
            T = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, k, k);
            C = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, k);
            required_tasks[0] = spotrf_tasks[k];
            if (k == 0) {
                cpu_lp->num_required = 1;
            } else {
                cpu_lp->num_required = 2;
                required_tasks[1] = sgemm_tasks[m][k][k-1];
            }
            cpu_lp->requires = required_tasks;
            strsm_tasks[m][k] = atmi_strsm_kernel(cpu_lp, m, k, PlasmaRight, PlasmaLower, PlasmaTrans, PlasmaNonUnit, tempmm, descA->nb, 1.0F, T, ldak, C, ldam);
        }
        
        for(m = k+1; m <= descA->mt-1; m++) {
            tempmm = m == descA->mt - 1 ? descA->m - m * descA->mb : descA->mb;
            ldam = ATMI_BLKLDD(descA, m);
            T = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, m);
            A = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, k);
            required_tasks[0] = strsm_tasks[m][k];
            if (k == 0) {
                cpu_lp->num_required = 1;
            } else {
                cpu_lp->num_required = 2;
                required_tasks[1] = ssyrk_tasks[k-1][m];
            }
            cpu_lp->requires = required_tasks;
            ssyrk_tasks[k][m] = atmi_ssyrk_kernel(cpu_lp, m, k, PlasmaLower, PlasmaNoTrans, tempmm, descA->mb, -1.0F, A, ldam, 1.0F, T, ldam);
            
            for(n = k+1; n < m; n++) {
                ldan = ATMI_BLKLDD(descA, n);
                A = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, k);
                B = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, n, k);
                C = (float *)sym_two_dim_block_cyclic_lookup_matrix(descA, m, n);
                required_tasks[0] = strsm_tasks[m][k];
                required_tasks[1] = strsm_tasks[n][k];
                if (k == 0) {
                    cpu_lp->num_required = 2;
                } else {
                    cpu_lp->num_required = 3;
                    required_tasks[2] = sgemm_tasks[m][n][k-1];
                }
                sgemm_tasks[m][n][k] = atmi_sgemm_kernel(cpu_lp, m, n, k, PlasmaNoTrans, PlasmaTrans, tempmm, descA->mb, descA->mb, -1.0F, A, ldam, B, ldan, 1.0F, C, ldam);
            }
        }
        
    }
    
    printf("Tasks are all enqueued\n");

    atmi_task_t* ret_task = spotrf_tasks[descA->mt-1];
    SYNC_TASK(ret_task);
    return 0;

}
