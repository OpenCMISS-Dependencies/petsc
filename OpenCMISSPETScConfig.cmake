# Init with whatever else configuration is provided by PETSc config run
include (${CMAKE_CURRENT_LIST_DIR}/PETScConfig.cmake)

# Fixed settings
SET(PETSC_HAVE_FORTRAN YES)
option(FORTRAN_MANGLING "${PROJECT_NAME} - Fortran mangling scheme (default Add_)" Add_)

# Setup the fortran libraries to be available for function checking as well
LIST(APPEND PETSC_PACKAGE_LIBS ${CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES})
LIST(APPEND PETSC_PACKAGE_INCLUDES ${CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES})
SET(PETSC_HAVE_CXX YES)
SET(PETSC_USE_SINGLE_LIBRARY 1)
# RT library
if (UNIX AND NOT APPLE)
    LIST(APPEND PETSC_PACKAGE_LIBS rt)
endif()

# Headers/functions lookup lists
include(${CMAKE_CURRENT_SOURCE_DIR}/Functions.cmake)
SET(SEARCHHEADERS )
SET(SEARCHFUNCTIONS )

# MPI
find_package(MPI REQUIRED)
set(PETSC_HAVE_MPI YES)
set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
set(CMAKE_Fortran_COMPILER ${MPI_Fortran_COMPILER})
set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
LIST(APPEND PETSC_PACKAGE_LIBS ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES} ${MPI_Fortran_LIBRARIES})
LIST(APPEND PETSC_PACKAGE_INCLUDES ${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH} ${MPI_Fortran_INCLUDE_PATH})
LIST(APPEND SEARCHFUNCTIONS MPI_Comm_spawn MPI_Type_get_envelope MPI_Type_get_extent MPI_Type_dup MPI_Init_thread
      MPI_Iallreduce MPI_Ibarrier MPI_Finalized MPI_Exscan MPIX_Iallreduce MPI_Win_create MPI_Alltoallw MPI_Type_create_indexed_block)

# LA packages
find_package(BLAS ${BLAS_VERSION} REQUIRED)
find_package(LAPACK ${LAPACK_VERSION} REQUIRED)
SET(PETSC_HAVE_BLASLAPACK YES)

# Valgrind - UNIX only
SET(PETSC_HAVE_VALGRIND NO)
if (UNIX)
    find_package(VALGRIND QUIET)
    if (VALGRIND_FOUND)
        message(STATUS "Found Valgrind: ${VALGRIND_INCLUDE_DIR}")
        SET(PETSC_HAVE_VALGRIND 1)
        LIST(APPEND PETSC_PACKAGE_INCLUDES ${VALGRIND_INCLUDE_DIR})
    endif()
endif()
# Sowing: Only for ftn-auto generation through bfort. Not needed with dependencies
set (PETSC_HAVE_SOWING NO)

# Set libraries etc that will be included at link time for function existence tests
SET(CMAKE_REQUIRED_LIBRARIES ${PETSC_PACKAGE_LIBS})
SET(CMAKE_REQUIRED_INCLUDES ${PETSC_PACKAGE_INCLUDES})
foreach(FDIR ${PETSC_PACKAGE_INCLUDES})
    SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${FDIR}")
endforeach()
#message(STATUS "Req. includes: ${CMAKE_REQUIRED_INCLUDES}")
#message(STATUS "Req. libs: ${CMAKE_REQUIRED_LIBRARIES}")
#message(STATUS "Req. flags: ${CMAKE_REQUIRED_FLAGS}")

# Threads
if (USE_THREADS)
    find_package(Threads QUIET)
    if (Threads_FOUND)
        SET(PETSC_HAVE_PTHREAD YES)
        LIST(APPEND PETSC_PACKAGE_LIBS ${CMAKE_THREAD_LIBS_INIT})
        trycompile(PETSC_HAVE_PTHREAD_BARRIER_T "#include <pthread.h>" "pthread_barrier_t *a;" c)
        trycompile(PETSC_HAVE_SCHED_CPU_SET_T "#include <sched.h>" "cpu_set_t *a;" c)
        LIST(APPEND SEARCHHEADERS sys/sysctl) 
    else()
        message(WARNING "Threading was requested (USE_THREADS=${USE_THREADS}), but package could not be found. Disabling.")
        SET(PETSC_HAVE_PTHREAD NO)
    endif()
endif()

########################################################
# Header availabilities
# This list is from config/PETSc/Configure.py
LIST(APPEND SEARCHHEADERS setjmp dos endian fcntl float io limits malloc
    pwd search strings unistd sys/sysinfo machine/endian sys/param sys/procfs sys/resource
    sys/systeminfo sys/times sys/utsname string stdlib sys/socket sys/wait netinet/in
    netdb Direct time Ws2tcpip sys/types WindowsX cxxabi float ieeefp stdint sched pthread mathimf)
SET(PETSCCONF_HAVE_HEADERS )
foreach(hdr ${SEARCHHEADERS})
    STRING(TOUPPER ${hdr} HDR)
    STRING(REPLACE "/" "_" HDR ${HDR})
    SET(VARNAME "PETSC_HAVE_${HDR}_H")
    CHECK_INCLUDE_FILES("${hdr}.h" ${VARNAME})
    if (${${VARNAME}})
        LIST(APPEND PETSCCONF_HAVE_HEADERS "#define ${VARNAME} 1")
    endif()
endforeach()
STRING(REPLACE ";" "\n\n" PETSCCONF_HAVE_HEADERS "${PETSCCONF_HAVE_HEADERS}")
#message(STATUS "Detected available headers: ${PETSCCONF_HAVE_HEADERS}")

########################################################
# Function availabilities
LIST(APPEND SEARCHFUNCTIONS access _access clock drand48 getcwd _getcwd getdomainname gethostname
    gettimeofday getwd memalign memmove mkstemp popen PXFGETARG rand getpagesize
    readlink realpath sigaction signal sigset usleep sleep _sleep socket times
    gethostbyname uname snprintf _snprintf lseek _lseek time fork stricmp 
    strcasecmp bzero dlopen dlsym dlclose dlerror get_nprocs sysctlbyname _set_output_format)
SET(PETSCCONF_HAVE_FUNCS )
foreach(func ${SEARCHFUNCTIONS})
    STRING(TOUPPER ${func} FUNC)
    SET(VARNAME "PETSC_HAVE_${FUNC}")
    CHECK_FUNCTION_EXISTS(${func} ${VARNAME})
    if (${${VARNAME}})
        LIST(APPEND PETSCCONF_HAVE_FUNCS "#define ${VARNAME} 1")
    endif()
endforeach()
# MPI extras - see Buildsystem/config/packages/MPI.py lines 781 ff
if (PETSC_HAVE_MPI)
    if (PETSC_HAVE_MPI_WIN_CREATE)
        SET(PETSC_HAVE_MPI_REPLACE 1)
    endif()
    if (NOT PETSC_HAVE_MPI_TYPE_CREATE_INDEXED_BLOCK)
        SET(PETSC_HAVE_MPI_ALLTOALLW NO)
    endif()
    CHECK_FUNCTION_EXISTS(MPIDI_CH3I_sock_set PETSC_HAVE_MPICH_CH3_SOCK)
    CHECK_FUNCTION_EXISTS(MPIDI_CH3I_sock_fixed_nbc_progress PETSC_HAVE_MPICH_CH3_SOCK_FIXED_NBC_PROGRESS)
    trycompile(PETSC_HAVE_MPI_COMBINER_DUP "#include <mpi.h>" "int combiner = MPI_COMBINER_DUP;" c)
    trycompile(PETSC_HAVE_MPI_COMBINER_CONTIGUOUS "#include <mpi.h>" "int combiner = MPI_COMBINER_CONTIGUOUS;" c)
    trycompile(PETSC_HAVE_MPI_COMM_F2C "#include <mpi.h>" "if (MPI_Comm_f2c((MPI_Fint)0));" c)
    trycompile(PETSC_HAVE_MPI_COMM_C2F "#include <mpi.h>" "if (MPI_Comm_c2f(MPI_COMM_WORLD));" c)
    trycompile(PETSC_HAVE_MPI_FINT "#include <mpi.h>" "MPI_Fint a;" c)
    trycompile(PETSC_HAVE_MPI_IN_PLACE "#include <mpi.h>" "if (MPI_Allreduce(MPI_IN_PLACE,0, 1, MPI_INT, MPI_SUM, MPI_COMM_SELF));" c)
    # Data types
    foreach(MPI_DATATYPE MPI_LONG_DOUBLE MPI_INT64_T MPI_C_DOUBLE_COMPLEX)
        trycompile(PETSC_HAVE_${MPI_DATATYPE} 
            "#ifdef PETSC_HAVE_STDLIB_H\n  #include <stdlib.h>\n#endif\n#include <mpi.h>\n"
            "MPI_Aint size;\nint ierr;\nMPI_Init(0,0);\nierr = MPI_Type_extent(${MPI_DATATYPE}, &size);\nif(ierr || (size == 0)) exit(1);\nMPI_Finalize();\n" c)
        if (PETSC_HAVE_${MPI_DATATYPE})
            LIST(APPEND PETSCCONF_HAVE_FUNCS "#define PETSC_HAVE_${MPI_DATATYPE} 1")
        endif()
    endforeach()
endif()
STRING(REPLACE ";" "\n\n" PETSCCONF_HAVE_FUNCS "${PETSCCONF_HAVE_FUNCS}")
#message(STATUS "Detected available functions: ${PETSCCONF_HAVE_FUNCS}")

# MPI-IO      
SET(_MPIIO_CODELIST "MPI_Aint lb, extent\;\nif (MPI_Type_get_extent(MPI_INT, &lb, &extent))\;\n"
    "MPI_File fh\;\nvoid *buf\;\nMPI_Status status\;\nif (MPI_File_write_all(fh, buf, 1, MPI_INT, &status))\;\n"
    "MPI_File fh\;\nvoid *buf\;\nMPI_Status status\;\nif (MPI_File_read_all(fh, buf, 1, MPI_INT, &status))\;\n"
    "MPI_File fh\;\nMPI_Offset disp\;\nMPI_Info info\;\nif (MPI_File_set_view(fh, disp, MPI_INT, MPI_INT, \"\", info))\;\n"
    "MPI_File fh\;\nMPI_Info info\;\nif (MPI_File_open(MPI_COMM_SELF, \"\", 0, info, &fh))\;\n"
    "MPI_File fh\;\nMPI_Info info\;\nif (MPI_File_close(&fh))\;\n")
SET(PETSC_HAVE_MPIIO YES)
foreach(_IDX RANGE 0 5)
    LIST(GET _MPIIO_CODELIST ${_IDX} _MPIIOCODE)
    trycompile(MPIIOCHECK${_IDX} "#include <mpi.h>" "${_MPIIOCODE}" c)
    if (NOT MPIIOCHECK${_IDX})
        SET(PETSC_HAVE_MPIIO NO)
        #break()
    endif()
endforeach()

########################################################
# Fortran interfacing - option above [and passed in as def by opencmiss]
if (FORTRAN_MANGLING STREQUAL Add_)
    SET(PETSC_HAVE_FORTRAN_UNDERSCORE YES)
elseif(FORTRAN_MANGLING STREQUAL UpCase)
    SET(PETSC_HAVE_FORTRAN_CAPS YES)
endif()
# Note: Not used anywhere but similar: PETSC_HAVE__GFORTRAN_IARGC (underscores!!)
CHECK_FUNCTION_EXISTS(_gfortran_iargc PETSC_HAVE_GFORTRAN_IARGC)
CHECK_FORTRAN_FUNCTION_EXISTS(get_command_argument PETSC_HAVE_FORTRAN_GET_COMMAND_ARGUMENT)
CHECK_FORTRAN_FUNCTION_EXISTS(getarg PETSC_HAVE_FORTRAN_GETARG)

########################################################
# 3rd party packages
# Define list of all external packages and their targets (=libraries)
SET(ALLEXT PASTIX MUMPS SUITESPARSE SCALAPACK PTSCOTCH
    SUPERLU SUNDIALS HYPRE SUPERLU_DIST PARMETIS)
SET(PARMETIS_TARGETS parmetis metis)
SET(PTSCOTCH_TARGETS scotch ptscotch esmumps ptesmumps)
SET(SUITESPARSE_TARGETS suitesparseconfig amd btf camd cholmod colamd ccolamd klu umfpack)
SET(PASTIX_TARGETS pastix)
SET(SCALAPACK_TARGETS scalapack)
SET(MUMPS_TARGETS dmumps mumps_common pord) #smumps
SET(SUPERLU_DIST_TARGETS superlu_dist)
SET(SUPERLU_TARGETS superlu)
SET(SUNDIALS_TARGETS sundials_cvode sundials_fcvode sundials_cvodes
    sundials_ida sundials_fida sundials_idas
    sundials_kinsol sundials_fkinsol
    sundials_nvecparallel sundials_nvecserial
    )
SET(HYPRE_TARGETS hypre)

SET(PETSCCONF_HAVE_FLAGS )
SET(PETSC_CONFIGINFO_STRING )
foreach(PACKAGE ${ALLEXT})
    # Define the option
    option(USE_${PACKAGE} "Build PETSc with ${PACKAGE}" ON)
    
    # See if we want to use it
    if (USE_${PACKAGE})
        find_package(${PACKAGE} ${${PACKAGE}_VERSION} QUIET)
        if (${PACKAGE}_FOUND)
            message(STATUS "Building PETSC with ${PACKAGE}...")
            # Set the petsc-have flag
            SET(PETSC_HAVE_${PACKAGE} YES)
            # Add targets to link targets list
            LIST(APPEND PETSC_PACKAGE_LIBS ${${PACKAGE}_TARGETS})
        endif()
    endif()
    LIST(APPEND PETSC_PACKAGE_LIBS ${MPI_C_LIBRARIES} ${MPI_Fortran_LIBRARIES})
    
    # If found, add definitions to header and information files
    if (PETSC_HAVE_${PACKAGE})
        # petscconfig.h
        LIST(APPEND PETSCCONF_HAVE_FLAGS "#ifndef PETSC_HAVE_${PACKAGE}\n#define PETSC_HAVE_${PACKAGE} 1\n#endif\n\n")
        
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
        LIST(APPEND PETSC_CONFIGINFO_STRING "--with-${pkgname}=1 --with-${pkgname}-lib=[${LIBRARIES}] --with-${pkgname}-include=[${INCLUDES}]")
    endif()
endforeach()

########################################################
# X11
find_package(X11 QUIET)
if (X11_FOUND)
    SET(PETSC_HAVE_X TRUE)
    LIST(APPEND PETSC_PACKAGE_LIBS ${X11_LIBRARIES})
    LIST(APPEND PETSC_PACKAGE_INCLUDES ${X11_INCLUDE_DIR})
endif()

########################################################
# SSL support
find_package(OpenSSL QUIET)
if (OPENSSL_FOUND)
    message(STATUS "Building PETSc with OpenSSL ${OPENSSL_VERSION}")
    LIST(APPEND PETSC_PACKAGE_LIBS ${OPENSSL_LIBRARIES})
    SET(PETSC_HAVE_SSL 1)
endif()

########################################################
# OpenMP
if (WITH_OPENMP)
  find_package(OpenMP)
  if (OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${OpenMP_Fortran_FLAGS}")
    SET(PETSC_HAVE_OPENMP 1)
  else()
    SET(PETSC_HAVE_OPENMP FALSE)
  endif()
endif()