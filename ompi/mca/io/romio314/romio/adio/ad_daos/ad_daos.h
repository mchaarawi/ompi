/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#ifndef AD_DAOS_INCLUDE
#define AD_DAOS_INCLUDE

#include <daos_types.h>
#include <daos_api.h>
#include <daos_array.h>

#include "adio.h"

struct ADIO_DAOS_cont {
    /** container uuid */
    uuid_t		uuid;
    /** container daos OH */
    daos_handle_t	coh;
    /** Array Object ID for the MPI file */
    daos_obj_id_t	oid;
    /** Array OH for the MPI file */
    daos_handle_t	oh;
    /** data to store in a dkey block */
    daos_size_t		stripe_size;
};

int ADIOI_DAOS_error_convert(int error);

void ADIOI_DAOS_Open(ADIO_File fd, int *error_code);
void ADIOI_DAOS_Close(ADIO_File fd, int *error_code);
void ADIOI_DAOS_ReadContig(ADIO_File fd, void *buf, int count,
			   MPI_Datatype datatype, int file_ptr_type,
			   ADIO_Offset offset, ADIO_Status *status,
			   int *error_code);
void ADIOI_DAOS_WriteContig(ADIO_File fd, void *buf, int count,
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Status *status,
			    int *error_code);
#endif
