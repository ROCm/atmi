#ifndef ATMI_LAPACK
#define ATMI_LAPACK 
int atmi_spotrf_L(PLASMA_enum uplo, tiled_matrix_desc_t *descA);
int atmi_task_spotrf_L(PLASMA_enum uplo, tiled_matrix_desc_t *descA);
#endif /* ATMI_LAPACK */
