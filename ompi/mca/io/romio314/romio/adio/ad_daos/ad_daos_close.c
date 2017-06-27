/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

void ADIOI_DAOS_Close(ADIO_File fd, int *error_code)
{
    struct ADIO_DAOS_cont *cont = (struct ADIO_DAOS_cont *)fd->fs_ptr;
    int rc;

    rc = daos_cont_close(cont->coh, NULL);
    if (rc == 0) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "Container Close failed", 0);
        goto err_cont;
    }

    ADIOI_Free(fd->fs_ptr);
    fd->fs_ptr = NULL;

    *error_code = MPI_SUCCESS;
}
/*
 * vim: ts=8 sts=4 sw=4 noexpandtab
 */
