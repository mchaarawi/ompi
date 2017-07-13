## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##

if BUILD_AD_DAOS

noinst_HEADERS += adio/ad_daos/ad_daos.h

romio_other_sources += \
    adio/ad_daos/ad_daos.c \
    adio/ad_daos/ad_daos_features.c \
    adio/ad_daos/ad_daos_common.c \
    adio/ad_daos/ad_daos_open.c \
    adio/ad_daos/ad_daos_close.c \
    adio/ad_daos/ad_daos_io.c

endif BUILD_AD_DAOS

