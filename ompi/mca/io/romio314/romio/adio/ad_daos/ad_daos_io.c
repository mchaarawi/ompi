/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

#include "../../mpi-io/mpioimpl.h"
#include "../../mpi-io/mpioprof.h"
#include "mpiu_greq.h"

enum {
    DAOS_WRITE,
    DAOS_READ
};

static MPIX_Grequest_class ADIOI_DAOS_greq_class = 0;
int ADIOI_DAOS_aio_free_fn(void *extra_state);
int ADIOI_DAOS_aio_poll_fn(void *extra_state, MPI_Status *status);
int ADIOI_DAOS_aio_wait_fn(int count, void ** array_of_states,
                           double timeout, MPI_Status *status);

static void DAOS_IOContig(ADIO_File fd, void * buf, int count,
			  MPI_Datatype datatype, int file_ptr_type,
			  ADIO_Offset offset, ADIO_Status *status,
			  MPI_Request *request, int flag, int *error_code)
{
    MPI_Count datatype_size;
    uint64_t len;
    daos_array_ranges_t ranges;
    daos_range_t rg;
    daos_sg_list_t sgl;
    daos_iov_t iov;
    int ret;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    ADIOI_AIO_Request *aio_req;
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

    if (request) {
        aio_req = (ADIOI_AIO_Request*)ADIOI_Calloc(sizeof(ADIOI_AIO_Request), 1);
        daos_event_init(&aio_req->daos_event, DAOS_HDL_INVAL, NULL);
    }

    if (flag == DAOS_WRITE) {
        ret = daos_array_write(cont->oh, cont->epoch, &ranges, &sgl, NULL, NULL);
        if (ret != 0) {
            printf("daos_array_write() failed with %d\n", ret);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(ret),
                                               "Error in daos_array_write", 0);
            return;
        }
    }
    else if (flag == DAOS_READ) {
        ret = daos_array_read(cont->oh, cont->epoch, &ranges, &sgl, NULL, NULL);
        if (ret != 0) {
            printf("daos_array_read() failed with %d\n", ret);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(ret),
                                               "Error in daos_array_read", 0);
            return;
        }
    }

    if (request) {
        if (ADIOI_DAOS_greq_class == 0) {
            MPIX_Grequest_class_create(ADIOI_GEN_aio_query_fn,
                                       ADIOI_DAOS_aio_free_fn, MPIU_Greq_cancel_fn,
                                       ADIOI_DAOS_aio_poll_fn, ADIOI_DAOS_aio_wait_fn,
                                       &ADIOI_DAOS_greq_class);
        }
        MPIX_Grequest_class_allocate(ADIOI_DAOS_greq_class, aio_req, request);
        memcpy(&(aio_req->req), request, sizeof(MPI_Request));
    }

#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event( ADIOI_MPE_write_b, 0, NULL );
#endif

    if (file_ptr_type == ADIO_INDIVIDUAL) {
	fd->fp_ind += len;
    }

    fd->fp_sys_posn = offset + len;

#ifdef HAVE_STATUS_SET_BYTES
    if (status)
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
                  offset, status, NULL, DAOS_READ, error_code);
}

void ADIOI_DAOS_WriteContig(ADIO_File fd, const void *buf, int count,
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, ADIO_Status *status,
			    int *error_code)
{
    DAOS_IOContig(fd, buf, count, datatype, file_ptr_type,
		  offset, status, NULL, DAOS_WRITE, error_code);
}

void ADIOI_DAOS_IReadContig(ADIO_File fd, void *buf, int count,
			    MPI_Datatype datatype, int file_ptr_type,
			    ADIO_Offset offset, MPI_Request *request,
			    int *error_code)
{
    DAOS_IOContig(fd, buf, count, datatype, file_ptr_type,
                  offset, NULL, request, DAOS_READ, error_code);
}

void ADIOI_DAOS_IWriteContig(ADIO_File fd, const void *buf, int count,
                             MPI_Datatype datatype, int file_ptr_type,
                             ADIO_Offset offset, MPI_Request *request,
                             int *error_code)
{
    DAOS_IOContig(fd, (void *)buf, count, datatype, file_ptr_type,
                  offset, NULL, request, DAOS_WRITE, error_code);
}

int ADIOI_DAOS_aio_free_fn(void *extra_state)
{
    ADIOI_AIO_Request *aio_req;
    aio_req = (ADIOI_AIO_Request*)extra_state;

    ADIOI_Free(aio_req);

    return MPI_SUCCESS;
}

int ADIOI_DAOS_aio_poll_fn(void *extra_state, MPI_Status *status)
{
    ADIOI_AIO_Request *aio_req;
    int ret;
    bool flag;

    aio_req = (ADIOI_AIO_Request *)extra_state;

    ret = daos_event_test(&aio_req->daos_event, DAOS_EQ_NOWAIT, &flag);
    if (ret != 0)
        return MPI_UNDEFINED;

    if (flag)
	MPI_Grequest_complete(aio_req->req);

    return MPI_SUCCESS;
}

/* wait for multiple requests to complete */
int ADIOI_DAOS_aio_wait_fn(int count, void ** array_of_states,
                           double timeout, MPI_Status *status)
{

    ADIOI_AIO_Request **aio_reqlist;
    int i, ret;

    aio_reqlist = (ADIOI_AIO_Request **)array_of_states;

    /** MSC - need to figure our exact symantics of the wait call */
    for (i = 0; i < count; i++) {
        bool flag;

        ret = daos_event_test(&aio_reqlist[i]->daos_event,
                              (int64_t)timeout, &flag);
        if (ret != 0)
            return MPI_UNDEFINED;

        if (flag)
            MPI_Grequest_complete(aio_reqlist[i]->req);
    }

    return MPI_SUCCESS; /* TODO: no idea how to deal with errors */
}
