# Simply use whatever else configuration is provided by PETSc config run
include (./PETScConfig.cmake)

# Fixed settings
SET(PETSC_HAVE_FORTRAN YES)
SET(PETSC_HAVE_CXX YES)
SET(PETSC_USE_SINGLE_LIBRARY 1)
SET(BUILD_SHARED_LIBS NO)

find_package(BLAS CONFIG REQUIRED)
find_package(LAPACK CONFIG REQUIRED)
SET(PETSC_HAVE_BLASLAPACK YES)

macro(CHECKEXTERN NAME)
    if (USE_${NAME})
        find_package(${NAME} CONFIG REQUIRED)
        SET(PETSC_HAVE_${NAME} YES)
        LIST(APPEND PETSC_PACKAGE_LIBS ${ARGN})
endmacro()

CHECKEXTERN(METIS metis)
CHECKEXTERN(PARMETIS parmetis metis)
CHECKEXTERN(PTSCOTCH scotch ptscotch)
CHECKEXTERN(SUITESPARSE suitesparseconfig amd btf camd cholmod colamd ccolamd klu umfpack)
CHECKEXTERN(PASTIX pastix)
CHECKEXTERN(MUMPS mumps)
CHECKEXTERN(SUPERLU_DIST superlu_dist)
CHECKEXTERN(SUPERLU superlu)
CHECKEXTERN(SUNDIALS sundials_cvode sundials_fcvode sundials_cvodes
    sundials_ida sundials_fida sundials_idas
    sundials_kinsol sundials_fkinsol
    sundials_nvecparallel sundials_nvecserial
    )
CHECKEXTERN(HYPRE hypre)