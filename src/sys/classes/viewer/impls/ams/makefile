#requirespackage  'PETSC_HAVE_SAWS'

ALL: lib

CFLAGS   = -DSAWS_HOME='"${SAWS_HOME}"'
FFLAGS   =
SOURCEC  = ams.c amsopen.c
SOURCEF  =
SOURCEH  = ../../../../../../include/petscviewersaws.h
MANSEC   = Viewer
LIBBASE  = libpetscsys
DIRS     =
LOCDIR   = src/sys/classes/viewer/impls/ams/

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules
include ${PETSC_DIR}/lib/petsc/conf/test




