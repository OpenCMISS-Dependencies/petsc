TYPE=release
OCMLIBM=/home/dwirtz/software/opencmiss/install/x86_64_linux/gnu-4.8.4/openmpi_release/static/$TYPE/lib
OCMINCM=/home/dwirtz/software/opencmiss/install/x86_64_linux/gnu-4.8.4/openmpi_release/static/$TYPE/include
OCMLIB=/home/dwirtz/software/opencmiss/install/x86_64_linux/gnu-4.8.4/no_mpi/static/$TYPE/lib
OCMINC=/home/dwirtz/software/opencmiss/install/x86_64_linux/gnu-4.8.4/no_mpi/static/$TYPE/include
DBGSUFF=
#
./configure --prefix=/home/dwirtz/software/petsc/install --with-cmake=1 \
--with-single-library=1 --with-shared-libraries=0 --with-precision=double --with-scalar-type=real \
--with-parmetis=1 --with-parmetis-include=$OCMINCM/parmetis-4.0.3 --with-parmetis-lib=$OCMLIBM/libparmetis-4.0.3$DBGSUFF.a \
--with-metis=1 --with-metis-include=$OCMINCM/metis-5.1 --with-metis-lib=$OCMLIBM/libmetis-5.1$DBGSUFF.a \
--with-ptscotch=1 --with-ptscotch-include=$OCMINCM --with-ptscotch-lib=\[$OCMLIBM/libptscotch-6.0.3$DBGSUFF.a,$OCMLIBM/libscotch-6.0.3$DBGSUFF.a,\
$OCMLIBM/libptesmumps-6.0.3$DBGSUFF.a,$OCMLIBM/libesmumps-6.0.3$DBGSUFF.a\] \
--with-suitesparse=1 --with-suitesparse-include=$OCMINCM \
--with-suitesparse-lib=\[$OCMLIBM/libumfpack-5.7.0$DBGSUFF.a,$OCMLIBM/libcholmod-3.0.1$DBGSUFF.a,$OCMLIBM/libamd$DBGSUFF.a,$OCMLIBM/libcamd$DBGSUFF.a,$OCMLIBM/libccolamd$DBGSUFF.a,$OCMLIBM/libklu$DBGSUFF.a,\
$OCMLIBM/libbtf$DBGSUFF.a,$OCMLIBM/libcolamd$DBGSUFF.a,$OCMLIBM/libsuitesparseconfig$DBGSUFF.a\] \
--with-pastix=1 --with-pastix-include=$OCMINCM/pastix/int32/d --with-pastix-lib=\[$OCMLIBM/libpastix-5.2.2.16$DBGSUFF.a,\
$OCMLIBM/libpastix-dmatrix-driver-5.2.2.16$DBGSUFF.a,$OCMLIBM/libpastix-smatrix-driver-5.2.2.16$DBGSUFF.a,\
$OCMLIBM/libmetis-5.1$DBGSUFF.a,$OCMLIBM/libptscotch-6.0.3$DBGSUFF.a,$OCMLIBM/libscotch-6.0.3$DBGSUFF.a\] \
--with-scalapack=1 --with-scalapack-include=$OCMINCM --with-scalapack-lib=$OCMLIBM/libscalapack-2.8$DBGSUFF.a \
--with-superlu=1 --with-superlu-include=$OCMINC/superlu --with-superlu-lib=$OCMLIB/libsuperlu-4.3$DBGSUFF.a \
--with-sundials=1 --with-sundials-include=$OCMINCM --with-sundials-lib=\[$OCMLIBM/libsundials_cvode$DBGSUFF.a,\
$OCMLIBM/libsundials_cvodes$DBGSUFF.a,$OCMLIBM/libsundials_fcvode$DBGSUFF.a,$OCMLIBM/libsundials_fida$DBGSUFF.a,$OCMLIBM/libsundials_fkinsol$DBGSUFF.a,\
$OCMLIBM/libsundials_fnvecparallel$DBGSUFF.a,$OCMLIBM/libsundials_ida$DBGSUFF.a,$OCMLIBM/libsundials_idas$DBGSUFF.a,$OCMLIBM/libsundials_kinsol$DBGSUFF.a,\
$OCMLIBM/libsundials_nvecparallel$DBGSUFF.a,$OCMLIBM/libsundials_nvecserial$DBGSUFF.a\] \
--with-hypre=1 --with-hypre-include=$OCMINCM/hypre --with-hypre-lib=$OCMLIBM/libhypre-2.10.0$DBGSUFF.a \
--with-superlu_dist=1 --with-superlu_dist-include=$OCMINCM/superlu_dist --with-superlu_dist-lib=$OCMLIBM/libsuperlu-dist-4.1$DBGSUFF.a \
--with-mumps=1 --with-mumps-lib=\[$OCMLIBM/libsmumps-5.0.0$DBGSUFF.a,$OCMLIBM/libdmumps-5.0.0$DBGSUFF.a,\
$OCMLIBM/libcmumps-5.0.0$DBGSUFF.a,$OCMLIBM/libzmumps-5.0.0$DBGSUFF.a,\
$OCMLIBM/libmumps_common-5.0.0$DBGSUFF,$OCMLIBM/libpord-5.0.0$DBGSUFF.a,\
$OCMLIBM/libptesmumps-6.0.3$DBGSUFF.a,$OCMLIBM/libesmumps-6.0.3$DBGSUFF.a\] \
--with-mumps-include=$OCMINCM/mumps


#
# make PETSC_DIR=/home/dwirtz/software/petsc PETSC_ARCH=arch-linux2-c-debug all
# make PETSC_DIR=/home/dwirtz/software/petsc PETSC_ARCH=arch-linux2-c-debug test
