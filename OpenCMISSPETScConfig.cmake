# Simply use whatever else configuration is provided by PETSc config run
include (./PETScConfig.cmake)

# Fixed settings
SET(PETSC_HAVE_FORTRAN YES)
# This should contain quadmath, m, gfortran
LIST(APPEND PETSC_PACKAGE_LIBS ${CMAKE_Fortran_IMPLICIT_LIBRARIES})
SET(PETSC_HAVE_CXX YES)
SET(PETSC_USE_SINGLE_LIBRARY 1)
SET(BUILD_SHARED_LIBS NO)

# MPI
find_package(MPI REQUIRED)
set(PETSC_HAVE_MPI YES)
set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
set(CMAKE_Fortran_COMPILER ${MPI_Fortran_COMPILER})
set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
#LIST(APPEND PETSC_PACKAGE_INCLUDES ${MPI_C_INCLUDE_PATH} ${MPI_Fortran_INCLUDE_PATH})
#LIST(APPEND PETSC_PACKAGE_LIBS ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES} ${MPI_Fortran_LIBRARIES})

find_package(BLAS CONFIG REQUIRED)
find_package(LAPACK CONFIG REQUIRED)
SET(PETSC_HAVE_BLASLAPACK YES)

# Define list of all external packages and their targets (=libraries)
SET(ALLEXT SCALAPACK PARMETIS PTSCOTCH SUITESPARSE PASTIX MUMPS SUPERLU SUNDIALS HYPRE) #SUPERLU_DIST
SET(PARMETIS_TARGETS parmetis metis)
SET(PTSCOTCH_TARGETS scotch ptscotch)
SET(SUITESPARSE_TARGETS suitesparseconfig amd btf camd cholmod colamd ccolamd klu umfpack)
SET(PASTIX_TARGETS pastix)
SET(SCALAPACK_TARGETS scalapack)
SET(MUMPS_TARGETS mumps)
SET(SUPERLU_DIST_TARGETS superlu_dist)
SET(SUPERLU_TARGETS superlu)
SET(SUNDIALS_TARGETS sundials_cvode sundials_fcvode sundials_cvodes
    sundials_ida sundials_fida sundials_idas
    sundials_kinsol sundials_fkinsol
    sundials_nvecparallel sundials_nvecserial
    )
SET(HYPRE_TARGETS hypre)

SET(PETSCCONF_HAVE_FLAGS )
SET(PETSC_CONFIG_DEFS )
foreach(PACKAGE ${ALLEXT})
    # Define the option
    option(USE_${PACKAGE} "Build PETSc with ${PACKAGE}" ON)
    
    # See if we want to use it
    if (USE_${PACKAGE})
        find_package(${PACKAGE} CONFIG REQUIRED)
        # Set the petsc-have flag
        SET(PETSC_HAVE_${PACKAGE} YES)
        # Add targets to link targets list
        LIST(APPEND PETSC_PACKAGE_LIBS ${${PACKAGE}_TARGETS})
    endif()
    
    # If found, add definitions to header and information files
    if (PETSC_HAVE_${PACKAGE})
        # petscconfig.h
        LIST(APPEND PETSCCONF_HAVE_FLAGS "#ifndef PETSC_HAVE_${PACKAGE}\n#define PETSC_HAVE_${PACKAGE} 1\n#endif\n\n")
        STRING(REPLACE ";" "" PETSCCONF_HAVE_FLAGS "${PETSCCONF_HAVE_FLAGS}")
        # petscconfiginfo.h
        STRING(TOLOWER ${PACKAGE} pkgname)
        SET(INCLUDES )
        SET(LIBRARIES )
        foreach(TARGET ${${PACKAGE}_TARGETS})
            get_target_property(INCDIR ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
            LIST(APPEND INCLUDES ${INCDIR})
            get_target_property(TARGET_FILE ${TARGET} LOCATION)
            LIST(APPEND LIBRARIES ${TARGET_FILE})
        endforeach()
        STRING(REPLACE ";" "," LIBRARIES "${LIBRARIES}")
        STRING(REPLACE ";" "," INCLUDES "${INCLUDES}")
        LIST(APPEND PETSC_CONFIG_DEFS "--with-${pkgname}=1 --with-${pkgname}-lib=[${LIBRARIES}] --with-${pkgname}-include=[${INCLUDES}]")
        STRING(REPLACE ";" " " PETSC_CONFIG_DEFS "${PETSC_CONFIG_DEFS}")
    endif()
endforeach()
# Configure the build-dependent header files
configure_file(${PETSc_SOURCE_DIR}/include/petscconf.h.in ${PETSc_BINARY_DIR}/include/petscconf.h)
configure_file(${PETSc_SOURCE_DIR}/include/petscconfiginfo.h.in ${PETSc_BINARY_DIR}/include/petscconfiginfo.h)
configure_file(${PETSc_SOURCE_DIR}/include/petscfix.h.in ${PETSc_BINARY_DIR}/include/petscfix.h COPYONLY)
configure_file(${PETSc_SOURCE_DIR}/include/petscmachineinfo.h.in ${PETSc_BINARY_DIR}/include/petscmachineinfo.h)