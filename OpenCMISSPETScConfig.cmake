# Init with whatever else configuration is provided by PETSc config run
include (${CMAKE_CURRENT_LIST_DIR}/PETScConfig.cmake)

# Fixed settings
SET(PETSC_HAVE_FORTRAN YES)
option(FORTRAN_MANGLING "${PROJECT_NAME} - Fortran mangling scheme (default Add_)" Add_)
option(PETSC_USE_DEBUG "${PROJECT_NAME} - Build with DEBUG information" NO)

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
if (MPI_C_COMPILER)
    set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
endif()
if (MPI_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
endif()
if (MPI_Fortran_COMPILER)
    set(CMAKE_Fortran_COMPILER ${MPI_Fortran_COMPILER})
endif()

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
set(PETSC_HAVE_SOWING NO)

# Socket stuff (missing.py:65)
if (WIN32)
    list(APPEND PETSC_PACKAGE_LIBS Ws2_32)
endif()

# Set libraries etc that will be included at link time for function existence tests
#message(STATUS "PETSC Include dirs: ${PETSC_PACKAGE_INCLUDES}")#\nInclude Libraries: ${PETSC_PACKAGE_LIBS}
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
        CHECK_SYMBOL_EXISTS(pthread_barrier_t pthread.h PETSC_HAVE_PTHREAD_BARRIER_T)
        #trycompile(PETSC_HAVE_PTHREAD_BARRIER_T "#include <pthread.h>" "pthread_barrier_t *a;" c)
        CHECK_SYMBOL_EXISTS(cpu_set_t sched.h PETSC_HAVE_SCHED_CPU_SET_T)
        #trycompile(PETSC_HAVE_SCHED_CPU_SET_T "#include <sched.h>" "cpu_set_t *a;" c)
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
    netdb Direct time Ws2tcpip sys/types WindowsX cxxabi float ieeefp stdint sched pthread mathimf
    signal dlfcn linux_header math sys/time fenv Winsock2)
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

# check availability of uid_t and gid_t
if (MINGW)
    if (PETSC_HAVE_UNISTD_H)
        CHECK_SYMBOL_EXISTS(uid_t "unistd.h" PETSC_HAVE_UID_T)
        CHECK_SYMBOL_EXISTS(gid_t "unistd.h" PETSC_HAVE_GID_T)
    elseif (PETSC_HAVE_SYS_TYPES_H)
        CHECK_SYMBOL_EXISTS(uid_t "sys/types.h" PETSC_HAVE_UID_T)
        CHECK_SYMBOL_EXISTS(gid_t "sys/types.h" PETSC_HAVE_GID_T)
    endif()
endif()

# __SSE__
CHECK_SYMBOL_EXISTS(__SSE__ "" PETSC_HAVE_SSE)
if (MINGW AND NOT PETSC_HAVE_SSE)
    set(PETSC_HAVE_XMMINTRIN_H NO)
endif()

########################################################
# Signal availabilities
set(SEARCHSIGNALS ABRT ALRM BUS CHLD CONT FPE HUP ILL INT KILL PIPE QUIT SEGV
    STOP SYS TERM TRAP TSTP URG USR1 USR2)
SET(PETSCCONF_HAVE_SIGNAL )
foreach(sig ${SEARCHSIGNALS})
    if (PETSC_HAVE_SIGNAL_H)
        CHECK_SYMBOL_EXISTS("SIG${sig}" "signal.h" PETSC_HAVE_SIG${sig})
    endif()
    if (NOT PETSC_HAVE_SIG${sig})
        LIST(APPEND PETSCCONF_HAVE_SIGNAL "#define PETSC_MISSING_SIG${sig} 1")
    endif()
endforeach()
STRING(REPLACE ";" "\n\n" PETSCCONF_HAVE_SIGNAL "${PETSCCONF_HAVE_SIGNAL}")
#message(STATUS "Detected available headers: ${PETSCCONF_HAVE_SIGNAL}")
if (PETSC_HAVE_SIGNAL_H)
    CHECK_SYMBOL_EXISTS(siginfo_t "signal.h" PETSC_HAVE_SIGINFO_T)
endif()

########################################################
# Symbol availabilities
CHECK_SYMBOL_EXISTS(__int64 "" PETSC_HAVE___INT64)
if (PETSC_HAVE_WINSOCK2_H)
    CHECK_SYMBOL_EXISTS(socklen_t "Winsock2.h" PETSC_HAVE_SOCKLEN_T)
endif()

########################################################
# Function availabilities
LIST(APPEND SEARCHFUNCTIONS access _access clock drand48 getcwd _getcwd getdomainname gethostname
    gettimeofday getwd memalign memmove mkstemp popen PXFGETARG rand getpagesize
    readlink realpath sigaction signal sigset usleep sleep _sleep socket times
    gethostbyname uname snprintf _snprintf lseek _lseek time fork stricmp 
    strcasecmp bzero dlopen dlsym dlclose dlerror get_nprocs sysctlbyname _set_output_format
    closesocket WSAGetLastError # from missing.py:65 / socket stuff
)
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
    
    # The CheckSymbolExists wont find enum types - see http://www.cmake.org/cmake/help/v3.2/module/CheckSymbolExists.html
    #CHECK_SYMBOL_EXISTS(MPI_COMBINER_DUP mpi.h PETSC_HAVE_MPI_COMBINER_DUP)
    trycompile(PETSC_HAVE_MPI_COMBINER_DUP "#include <mpi.h>" "int combiner = MPI_COMBINER_DUP;" c)
    #CHECK_SYMBOL_EXISTS(MPI_COMBINER_CONTIGUOUS mpi.h PETSC_HAVE_MPI_COMBINER_CONTIGUOUS)
    trycompile(PETSC_HAVE_MPI_COMBINER_CONTIGUOUS "#include <mpi.h>" "int combiner = MPI_COMBINER_CONTIGUOUS;" c)
    
    CHECK_SYMBOL_EXISTS(MPI_Comm_f2c mpi.h PETSC_HAVE_MPI_COMM_F2C)
    #trycompile(PETSC_HAVE_MPI_COMM_F2C "#include <mpi.h>" "if (MPI_Comm_f2c((MPI_Fint)0));" c)
    
    CHECK_SYMBOL_EXISTS(MPI_Comm_c2f mpi.h PETSC_HAVE_MPI_COMM_C2F)
    #trycompile(PETSC_HAVE_MPI_COMM_C2F "#include <mpi.h>" "if (MPI_Comm_c2f(MPI_COMM_WORLD));" c)
    
    # MPI_Fint is a typedef - - see http://www.cmake.org/cmake/help/v3.2/module/CheckSymbolExists.html
    #CHECK_SYMBOL_EXISTS(MPI_Fint mpi.h PETSC_HAVE_MPI_FINT)
    trycompile(PETSC_HAVE_MPI_FINT "#include <mpi.h>" "MPI_Fint a;" c)
    
    CHECK_SYMBOL_EXISTS(MPI_IN_PLACE mpi.h PETSC_HAVE_MPI_IN_PLACE)
    
    # Data types
    foreach(MPI_DATATYPE MPI_LONG_DOUBLE MPI_INT64_T MPI_C_DOUBLE_COMPLEX)
        #trycompile(PETSC_HAVE_${MPI_DATATYPE} 
        #    "#ifdef PETSC_HAVE_STDLIB_H\n  #include <stdlib.h>\n#endif\n#include <mpi.h>\n"
        #    "MPI_Aint size;\nint ierr;\nMPI_Init(0,0);\nierr = MPI_Type_extent(${MPI_DATATYPE}, &size);\nif(ierr || (size == 0)) exit(1);\nMPI_Finalize();\n" c)
        CHECK_SYMBOL_EXISTS(${MPI_DATATYPE} mpi.h PETSC_HAVE_${MPI_DATATYPE})
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
# Prefetch config
set(PETSC_Prefetch_VAL "")
set(PETSC_Prefetch_KEY "PETSC_Prefetch(a,b,c)")
if (NOT SOLARIS AND NOT APPLE)
    trycompile(PREFETCH_TEST_1 "#include <xmmintrin.h>" "void *v = 0;_mm_prefetch((const char*)v,_MM_HINT_NTA);" c)
    if (PREFETCH_TEST_1)
        set(PETSC_HAVE_XMMINTRIN_H YES)
        set(PETSC_Prefetch_VAL "_mm_prefetch((const char*)(a),(c))")
        set(PETSC_PREFETCH_HINT_NTA YES)
        set(PETSC_PREFETCH_HINT_NTA_VAL _MM_HINT_NTA)
        set(PETSC_PREFETCH_HINT_T0 _MM_HINT_T0)
        set(PETSC_PREFETCH_HINT_T1 _MM_HINT_T1)
        set(PETSC_PREFETCH_HINT_T2 _MM_HINT_T2)
    else()
        trycompile(PREFETCH_TEST_2 "#include <xmmintrin.h>" "void *v = 0;_mm_prefetch(v,_MM_HINT_NTA);" c)
        if (PREFETCH_TEST_2)
            set(PETSC_HAVE_XMMINTRIN_H YES)
            set(PETSC_Prefetch_VAL "_mm_prefetch((const void*)(a),(c))")
            set(PETSC_PREFETCH_HINT_NTA YES)
            set(PETSC_PREFETCH_HINT_NTA_VAL _MM_HINT_NTA)
            set(PETSC_PREFETCH_HINT_T0 _MM_HINT_T0)
            set(PETSC_PREFETCH_HINT_T1 _MM_HINT_T1)
            set(PETSC_PREFETCH_HINT_T2 _MM_HINT_T2)
        else()
            trycompile(PREFETCH_TEST_3 "" "void *v = 0;__builtin_prefetch(v,0,0);" c)
            if (PREFETCH_TEST_3)
                set(PETSC_Prefetch_VAL "__builtin_prefetch((a),(b),(c))")
                set(PETSC_PREFETCH_HINT_NTA YES)
                set(PETSC_PREFETCH_HINT_NTA_VAL "0")
                set(PETSC_PREFETCH_HINT_T0 3)
                set(PETSC_PREFETCH_HINT_T1 2)
                set(PETSC_PREFETCH_HINT_T2 1)
            endif()
        endif()
    endif()
endif()

########################################################
# 3rd party packages
# Define list of all external packages and their targets (=libraries)
#
# This should ideally be contained somehow in the exported config files, but as it isnt we need
# to manually say which targets are defined in which package (which is knowledge that should be available
# also in this local context as we're consuming all of those packages here)
SET(PETSC_DEPENDENCIES PASTIX MUMPS SUITESPARSE SCALAPACK PTSCOTCH
    SUPERLU SUNDIALS HYPRE SUPERLU_DIST PARMETIS)
SET(PETSCCONF_HAVE_FLAGS )
SET(PETSC_CONFIGINFO_STRING )
foreach(PACKAGE ${PETSC_DEPENDENCIES})
    # Define the option
    option(USE_${PACKAGE} "Build PETSc with ${PACKAGE}" ON)
    
    # See if we want to use it
    if (USE_${PACKAGE})
        message(STATUS "Building PETSC with ${PACKAGE}...")
        find_package(${PACKAGE} ${${PACKAGE}_VERSION} REQUIRED)
        STRING(TOLOWER ${PACKAGE} pkgname)
        
        # Set the petsc-have flag
        SET(PETSC_HAVE_${PACKAGE} YES)
        
        # Add targets to link targets list
        LIST(APPEND PETSC_PACKAGE_LIBS ${pkgname})
        
        # petscconfig.h
        LIST(APPEND PETSCCONF_HAVE_FLAGS "#ifndef PETSC_HAVE_${PACKAGE}\n#define PETSC_HAVE_${PACKAGE} 1\n#endif\n\n")
        
        # petscconfiginfo.h
        SET(INCLUDES )
        SET(LIBRARIES )
        get_target_property(INCDIR ${pkgname} INTERFACE_INCLUDE_DIRECTORIES)
        LIST(APPEND INCLUDES ${INCDIR})
        get_target_property(LINK_LIBS ${pkgname} INTERFACE_LINK_LIBRARIES)
        LIST(APPEND LIBRARIES ${LINK_LIBS})
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
    set(WITH_OPENMP FALSE)
    SET(PETSC_HAVE_OPENMP FALSE)
  endif()
endif()