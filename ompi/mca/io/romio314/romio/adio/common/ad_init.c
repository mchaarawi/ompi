/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *   Copyright (C) 1997 University of Chicago.
 *   See COPYRIGHT notice in top-level directory.
 */

#include "adio.h"
#include "adio_extern.h"

#ifdef ROMIO_DAOS
#include <daos_types.h>
#include <daos_api.h>
#include <crt_util/common.h>

daos_handle_t daos_pool_oh;
#endif /* ROMIO_DAOS */


ADIOI_Flatlist_node *ADIOI_Flatlist = NULL;
ADIOI_Datarep *ADIOI_Datarep_head = NULL;
    /* list of datareps registered by the user */

/* for f2c and c2f conversion */
ADIO_File *ADIOI_Ftable = NULL;
int ADIOI_Ftable_ptr = 0, ADIOI_Ftable_max = 0;
ADIO_Request *ADIOI_Reqtable = NULL;
int ADIOI_Reqtable_ptr = 0, ADIOI_Reqtable_max = 0;
#ifndef HAVE_MPI_INFO
MPI_Info *MPIR_Infotable = NULL;
int MPIR_Infotable_ptr = 0, MPIR_Infotable_max = 0;
#endif

MPI_Info ADIOI_syshints = MPI_INFO_NULL;

MPI_Op ADIO_same_amode=MPI_OP_NULL;

#if defined(ROMIO_XFS) || defined(ROMIO_LUSTRE)
int ADIOI_Direct_read = 0, ADIOI_Direct_write = 0;
#endif

int ADIO_Init_keyval=MPI_KEYVAL_INVALID;

MPI_Errhandler ADIOI_DFLT_ERR_HANDLER = MPI_ERRORS_RETURN;


static void my_consensus(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype)
{
    int i, *in, *inout;
    in = (int*)invec;
    inout = (int*)inoutvec;

    for (i=0; i< *len; i++) {
        if (in[i] != inout[i])
	    inout[i] = ADIO_AMODE_NOMATCH;
    }
    return;
}

/* MSC - Make a generic DAOS function instead */
static daos_rank_list_t *
daos_rank_list_parse(const char *str, const char *sep)
{
	daos_rank_t	       *buf;
	int			cap = 8;
	daos_rank_list_t       *ranks = NULL;
	char		       *s;
	char		       *p;
	int			n = 0;

	buf = ADIOI_Malloc(sizeof(*buf) * cap);
	if (buf == NULL)
            goto out;
	s = strdup(str);
	if (s == NULL)
            goto out_buf;

	while ((s = strtok_r(s, sep, &p)) != NULL) {
		if (n == cap) {
			daos_rank_t    *buf_new;
			int		cap_new;

			/* Double the buffer. */
			cap_new = cap * 2;
			buf_new = ADIOI_Malloc(sizeof(*buf_new) * cap_new);
			if (buf_new == NULL)
                            goto out_s;
			memcpy(buf_new, buf, sizeof(*buf_new) * n);
			free(buf);
			buf = buf_new;
			cap = cap_new;
		}
		buf[n] = atoi(s);
		n++;
		s = NULL;
	}

	ranks = crt_rank_list_alloc(n);
	if (ranks == NULL)
            goto out_s;
	memcpy(ranks->rl_ranks, buf, sizeof(*buf) * n);

out_s:
	free(s);
out_buf:
	free(buf);
out:
	return ranks;
}

void ADIO_Init(int *argc, char ***argv, int *error_code)
{
#if defined(ROMIO_XFS) || defined(ROMIO_LUSTRE)
    char *c;
#endif

    ADIOI_UNREFERENCED_ARG(argc);
    ADIOI_UNREFERENCED_ARG(argv);

#ifdef ROMIO_INSIDE_MPICH
    MPIR_Ext_init();
#endif

/* initialize the linked list containing flattened datatypes */
    ADIOI_Flatlist = (ADIOI_Flatlist_node *) ADIOI_Malloc(sizeof(ADIOI_Flatlist_node));
    ADIOI_Flatlist->type = MPI_DATATYPE_NULL;
    ADIOI_Flatlist->next = NULL;
    ADIOI_Flatlist->blocklens = NULL;
    ADIOI_Flatlist->indices = NULL;

#if defined(ROMIO_XFS) || defined(ROMIO_LUSTRE)
    c = getenv("MPIO_DIRECT_READ");
    if (c && (!strcmp(c, "true") || !strcmp(c, "TRUE")))
	ADIOI_Direct_read = 1;
    else ADIOI_Direct_read = 0;
    c = getenv("MPIO_DIRECT_WRITE");
    if (c && (!strcmp(c, "true") || !strcmp(c, "TRUE")))
	ADIOI_Direct_write = 1;
    else ADIOI_Direct_write = 0;
#endif

#ifdef ROMIO_DAOS
    char *uuid_str;
    char *svcl_str;
    char *group;
    uuid_t pool_uuid;
    daos_pool_info_t pool_info;
    daos_rank_list_t *svcl = NULL;
    int rc;

    uuid_str = getenv ("DAOS_POOL");
    if (uuid_str != NULL) {
        if (uuid_parse(uuid_str, pool_uuid) < 0) {
            printf("Failed to parse pool UUID env\n");
            return;
        }
    }

    svcl_str = getenv ("DAOS_SVCL");
    if (svcl_str != NULL) {
        svcl = daos_rank_list_parse(svcl_str, ":");
        if (svcl == NULL) {
            printf("Failed to parse SVC list env\n");
            return;
        }
    }

    group = getenv ("DAOS_GROUP");
    rc = daos_pool_connect(pool_uuid, group, svcl, DAOS_PC_RW, &daos_pool_oh,
                           &pool_info, NULL);
    if (rc < 0)
        printf("Failed to connect to pool\n");

    if (group)
        free(group);
    if (svcl)
        crt_rank_list_free(svcl);
    if (svcl_str)
        free(svcl_str);
    if (uuid_str)
        free(uuid_str);
#endif

#ifdef ADIOI_MPE_LOGGING
    {
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_open_a, &ADIOI_MPE_open_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_read_a, &ADIOI_MPE_read_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_write_a, &ADIOI_MPE_write_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_lseek_a, &ADIOI_MPE_lseek_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_close_a, &ADIOI_MPE_close_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_writelock_a,
                                    &ADIOI_MPE_writelock_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_readlock_a,
                                    &ADIOI_MPE_readlock_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_unlock_a, &ADIOI_MPE_unlock_b );
        MPE_Log_get_state_eventIDs( &ADIOI_MPE_postwrite_a,
                                    &ADIOI_MPE_postwrite_b );
	MPE_Log_get_state_eventIDs( &ADIOI_MPE_openinternal_a,
			&ADIOI_MPE_openinternal_b);
	MPE_Log_get_state_eventIDs( &ADIOI_MPE_stat_a, &ADIOI_MPE_stat_b);
	MPE_Log_get_state_eventIDs( &ADIOI_MPE_iread_a, &ADIOI_MPE_iread_b);
	MPE_Log_get_state_eventIDs( &ADIOI_MPE_iwrite_a, &ADIOI_MPE_iwrite_b);

        int  comm_world_rank;
        MPI_Comm_rank( MPI_COMM_WORLD, &comm_world_rank );

        if ( comm_world_rank == 0 ) {
            MPE_Describe_state( ADIOI_MPE_open_a, ADIOI_MPE_open_b,
                                "open", "orange" );
            MPE_Describe_state( ADIOI_MPE_read_a, ADIOI_MPE_read_b,
                                "read", "green" );
            MPE_Describe_state( ADIOI_MPE_write_a, ADIOI_MPE_write_b,
                                "write", "blue" );
            MPE_Describe_state( ADIOI_MPE_lseek_a, ADIOI_MPE_lseek_b,
                                "lseek", "red" );
            MPE_Describe_state( ADIOI_MPE_close_a, ADIOI_MPE_close_b,
                                "close", "grey" );
            MPE_Describe_state( ADIOI_MPE_writelock_a, ADIOI_MPE_writelock_b,
                                "writelock", "plum" );
            MPE_Describe_state( ADIOI_MPE_readlock_a, ADIOI_MPE_readlock_b,
                                "readlock", "magenta" );
            MPE_Describe_state( ADIOI_MPE_unlock_a, ADIOI_MPE_unlock_b,
                                "unlock", "purple" );
            MPE_Describe_state( ADIOI_MPE_postwrite_a, ADIOI_MPE_postwrite_b,
                                "postwrite", "ivory" );
	    MPE_Describe_state( ADIOI_MPE_openinternal_a, ADIOI_MPE_openinternal_b, "open system", "blue");
	    MPE_Describe_state( ADIOI_MPE_stat_a, ADIOI_MPE_stat_b, "stat", "purple");
	    MPE_Describe_state( ADIOI_MPE_iread_a, ADIOI_MPE_iread_b, "iread", "purple");
	    MPE_Describe_state( ADIOI_MPE_iwrite_a, ADIOI_MPE_iwrite_b, "iwrite", "purple");
        }
    }
#endif

    *error_code = MPI_SUCCESS;
    MPI_Op_create(my_consensus, 1, &ADIO_same_amode);
}
