# Simply use whatever else configuration is provided by PETSc config run
include (./PETScConfig.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/petscconf.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/petscconf.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/petscconfiginfo.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/petscconfiginfo.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/petscfix.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/petscfix.h COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/petscmachineinfo.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/petscmachineinfo.h COPYONLY)

# Fixed settings
SET(PETSC_HAVE_FORTRAN YES)
SET(PETSC_HAVE_CXX YES)
SET(PETSC_USE_SINGLE_LIBRARY 1)
SET(BUILD_SHARED_LIBS NO)

find_package(MPI REQUIRED)
find_package(BLAS CONFIG REQUIRED)
find_package(LAPACK CONFIG REQUIRED)
SET(PETSC_HAVE_BLASLAPACK YES)

macro(CHECKEXTERN NAME)
    option(USE_${NAME} "Build PETSc with ${NAME}" ON)
    if (USE_${NAME})
        find_package(${NAME} CONFIG REQUIRED)
        SET(PETSC_HAVE_${NAME} YES)
        LIST(APPEND PETSC_PACKAGE_LIBS ${ARGN})
    endif()
endmacro()

CHECKEXTERN(PARMETIS parmetis metis)
CHECKEXTERN(PTSCOTCH scotch ptscotch)
CHECKEXTERN(SUITESPARSE suitesparseconfig amd btf camd cholmod colamd ccolamd klu umfpack)
CHECKEXTERN(PASTIX pastix)
CHECKEXTERN(SCALAPACK scalapack)
CHECKEXTERN(MUMPS mumps)
#CHECKEXTERN(SUPERLU_DIST superlu_dist)
CHECKEXTERN(SUPERLU superlu)
CHECKEXTERN(SUNDIALS sundials_cvode sundials_fcvode sundials_cvodes
    sundials_ida sundials_fida sundials_idas
    sundials_kinsol sundials_fkinsol
    sundials_nvecparallel sundials_nvecserial
    )
CHECKEXTERN(HYPRE hypre)

#message(STATUS "All PETSC libs: ${PETSC_PACKAGE_LIBS}")