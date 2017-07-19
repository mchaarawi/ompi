/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

/* adioi.h has the ADIOI_Fns_struct define */
#include "adioi.h"

struct ADIOI_Fns_struct ADIO_DAOS_operations = {
    ADIOI_DAOS_Open, /* Open */
    ADIOI_DAOS_OpenColl, /* OpenColl */
    ADIOI_DAOS_ReadContig, /* ReadContig */
    ADIOI_DAOS_WriteContig, /* WriteContig */
    ADIOI_DAOS_ReadStrided, /* ReadStridedColl */
    ADIOI_DAOS_WriteStrided, /* WriteStridedColl */
    ADIOI_GEN_SeekIndividual, /* SeekIndividual */
    ADIOI_DAOS_Fcntl, /* Fcntl */
    ADIOI_GEN_SetInfo, /* SetInfo */
    ADIOI_DAOS_ReadStrided, /* ReadStrided */
    ADIOI_DAOS_WriteStrided, /* WriteStrided */
    ADIOI_DAOS_Close, /* Close */
#ifdef ROMIO_HAVE_WORKING_AIO
    ADIOI_GEN_IreadContig, /* IreadContig */
    ADIOI_GEN_IwriteContig, /* IwriteContig */
#else
    ADIOI_FAKE_IreadContig, /* IreadContig */
    ADIOI_FAKE_IwriteContig, /* IwriteContig */
#endif
    ADIOI_GEN_IODone, /* ReadDone */
    ADIOI_GEN_IODone, /* WriteDone */
    ADIOI_GEN_IOComplete, /* ReadComplete */
    ADIOI_GEN_IOComplete, /* WriteComplete */
    ADIOI_GEN_IreadStrided, /* IreadStrided */
    ADIOI_GEN_IwriteStrided, /* IwriteStrided */
    ADIOI_GEN_Flush, /* Flush */
    ADIOI_GEN_Resize, /* Resize */
    ADIOI_GEN_Delete, /* Delete */
    ADIOI_DAOS_Feature, /* Features */
    "DAOS: ROMIO driver for DAOS",
};
