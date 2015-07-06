#ifndef ATMI_LAPACK
#define ATMI_LAPACK 

int atmi_spotrf_L(PLASMA_enum uplo, tiled_matrix_desc_t *descA);
atmi_task_t* atmi_spotrf_L_create_task(PLASMA_enum uplo, tiled_matrix_desc_t *descA);
int atmi_spotrf_L_progress_task(atmi_task_t *task);

#endif /* ATMI_LAPACK */
