/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

void ADIOI_DAOS_Fcntl(ADIO_File fd, int flag, ADIO_Fcntl_t *fcntl_struct,
                      int *error_code)
{
    int ret;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_FCNTL";

    switch(flag) {
    case ADIO_FCNTL_GET_FSIZE:
	ret = daos_array_get_size(cont->oh, cont->epoch,
				  &fcntl_struct->fsize, NULL);
	if (ret != 0 ) {
	    /* --BEGIN ERROR HANDLING-- */
	    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
					       MPIR_ERR_RECOVERABLE,
					       myname, __LINE__,
					       ADIOI_DAOS_error_convert(ret),
					       "Error in daos_array_get_size", 0);
	    /* --END ERROR HANDLING-- */
	}
	else {
	    *error_code = MPI_SUCCESS;
	}
	return;

    case ADIO_FCNTL_SET_DISKSPACE:
    case ADIO_FCNTL_SET_ATOMICITY:
    default:
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_ARG,
					   "**flag", "**flag %d", flag);
    }
}
