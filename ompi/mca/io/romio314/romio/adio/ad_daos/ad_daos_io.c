/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

enum {
    DAOS_WRITE,
    DAOS_READ
};

static void DAOS_IOContig(ADIO_File fd, void * buf, int count,
			  MPI_Datatype datatype, int file_ptr_type,
			  ADIO_Offset offset, ADIO_Status *status,
			  int flag, int *error_code)
{
    MPI_Count datatype_size;
    uint64_t len;
    daos_array_ranges_t ranges;
    daos_range_t rg;
    daos_sg_list_t sgl;
    daos_iov_t iov;
    int ret;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_IOCONTIG";

    MPI_Type_size_x(datatype, &datatype_size);
    len = (ADIO_Offset)datatype_size * (ADIO_Offset)count;

    if (file_ptr_type == ADIO_INDIVIDUAL) {
	offset = fd->fp_ind;
    }

    /** set memory location */
    sgl.sg_nr.num = 1;
    daos_iov_set(&iov, buf, len);
    sgl.sg_iovs = &iov;

    /** set array location */
    ranges.ranges_nr = 1;
    rg.len = len;
    rg.index = offset;
    ranges.ranges = &rg;

#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event( ADIOI_MPE_write_a, 0, NULL );
#endif

    if (flag == DAOS_WRITE) {
	    ret = daos_array_write(cont->oh, 0, &ranges, &sgl, NULL, NULL);
	    if (ret != 0) {
		    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
						       MPIR_ERR_RECOVERABLE,
						       myname, __LINE__,
						       ADIOI_DAOS_error_convert(ret),
						       "Error in daos_array_write", 0);
		    return;
	    }
    }
    else if (flag == DAOS_READ) {
	    ret = daos_array_read(cont->oh, 0, &ranges, &sgl, NULL, NULL);
	    if (ret != 0) {
		    *error_code = MPIO_Err_create_code(MPI_SUCCESS,
						       MPIR_ERR_RECOVERABLE,
						       myname, __LINE__,
						       ADIOI_DAOS_error_convert(ret),
						       "Error in daos_array_read", 0);
		    return;
	    }
    }

#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event( ADIOI_MPE_write_b, 0, NULL );
#endif

    if (file_ptr_type == ADIO_INDIVIDUAL) {
	fd->fp_ind += len;
    }

    fd->fp_sys_posn = offset + len;

#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, datatype, len);
#endif

    *error_code = MPI_SUCCESS;
#ifdef AGGREGATION_PROFILE
    MPE_Log_event (5037, 0, NULL);
#endif
}

void ADIOI_DAOS_ReadContig(ADIO_File fd, void *buf, int count,
			   MPI_Datatype datatype, int file_ptr_type,
			   ADIO_Offset offset, ADIO_Status *status,
			   int *error_code)
{
    DAOS_IOContig(fd, buf, count, datatype, file_ptr_type,
		  offset, status, DAOS_READ, error_code);
}

void ADIOI_DAOS_WriteContig(ADIO_File fd, void *buf, int count,
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Status *status,
			    int *error_code)
{
    DAOS_IOContig(fd, buf, count, datatype, file_ptr_type,
		  offset, status, DAOS_WRITE, error_code);
}
