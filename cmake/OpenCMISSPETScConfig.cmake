# Init with whatever else configuration is provided by PETSc config run
include (${CMAKE_CURRENT_LIST_DIR}/PETScConfig.cmake)

option(PETSC_USE_DEBUG "${PROJECT_NAME} - Build with DEBUG information" NO)

# Fixed settings
set(PETSC_HAVE_CXX YES)
set(PETSC_HAVE_FORTRAN YES)
if (NOT DEFINED FORTRAN_MANGLING)
    set(FORTRAN_MANGLING Add_ CACHE STRING "${PROJECT_NAME} - Fortran mangling scheme")
endif()
set(PETSC_USE_SINGLE_LIBRARY 1)

# Headers/functions lookup lists
include(Functions)
set(SEARCHHEADERS )
set(SEARCHFUNCTIONS )

# MPI
find_package(MPI REQUIRED)
set(PETSC_HAVE_MPI YES)
if (NOT CMAKE_C_COMPILER STREQUAL MPI_C_COMPILER)
    list(APPEND PETSC_PACKAGE_LIBS ${MPI_Fortran_LIBRARIES} ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
    list(APPEND PETSC_PACKAGE_INCLUDES ${MPI_Fortran_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH} ${MPI_C_INCLUDE_PATH})
endif()
# Extra MPI-related functions
list(APPEND SEARCHFUNCTIONS MPI_Comm_spawn MPI_Type_get_envelope MPI_Type_get_extent MPI_Type_dup MPI_Init_thread
      MPI_Iallreduce MPI_Ibarrier MPI_Finalized MPI_Exscan MPIX_Iallreduce MPI_Win_create MPI_Alltoallw MPI_Type_create_indexed_block)

# LA packages
find_package(BLAS ${BLAS_VERSION} REQUIRED)
find_package(LAPACK ${LAPACK_VERSION} REQUIRED)
set(PETSC_HAVE_BLASLAPACK YES)
list(APPEND PETSC_PACKAGE_LIBS blas lapack)

# Valgrind - UNIX only
set(PETSC_HAVE_VALGRIND NO)
if (UNIX)
    find_package(VALGRIND QUIET)
    if (VALGRIND_FOUND)
        message(STATUS "Found Valgrind: ${VALGRIND_INCLUDE_DIR}")
        set(PETSC_HAVE_VALGRIND 1)
        list(APPEND PETSC_PACKAGE_INCLUDES ${VALGRIND_INCLUDE_DIR})
    endif()
endif()
# Dont need anything else but fortran stubs generation from bfort inside sowing.
set(PETSC_HAVE_SOWING NO)

# Socket stuff (missing.py:65)
if (WIN32)
    list(APPEND PETSC_PACKAGE_LIBS Ws2_32)
endif()

# Set libraries etc that will be included at link time for function existence tests
#message(STATUS "PETSC Include Libraries: ${PETSC_PACKAGE_LIBS}, Implicit fortran libs: ${CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES}")
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES})
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES} ${CMAKE_Fortran_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES})
foreach(_ll ${CMAKE_Fortran_IMPLICIT_LINK_DIRECTORIES})
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L\"${_ll}\"")
endforeach()
if (WIN32)
    list(APPEND CMAKE_REQUIRED_LIBRARIES ${PETSC_PACKAGE_LIBS})
    list(APPEND CMAKE_REQUIRED_INCLUDES ${PETSC_PACKAGE_INCLUDES})
endif()
#message(STATUS "Req. includes: ${CMAKE_REQUIRED_INCLUDES}")
#message(STATUS "Req. libs: ${CMAKE_REQUIRED_LIBRARIES}")
#message(STATUS "Req. flags: ${CMAKE_REQUIRED_FLAGS}")


# Threads
if (USE_THREADS)
    find_package(Threads QUIET)
    if (Threads_FOUND)
        set(PETSC_HAVE_PTHREAD YES)
        list(APPEND PETSC_PACKAGE_LIBS ${CMAKE_THREAD_LIBS_INIT})
        CHECK_SYMBOL_EXISTS(pthread_barrier_t pthread.h PETSC_HAVE_PTHREAD_BARRIER_T)
        CHECK_SYMBOL_EXISTS(cpu_set_t sched.h PETSC_HAVE_SCHED_CPU_SET_T)
        list(APPEND SEARCHHEADERS sys/sysctl) 
    else()
        message(WARNING "Threading was requested (USE_THREADS=${USE_THREADS}), but package could not be found. Disabling.")
        set(PETSC_HAVE_PTHREAD NO)
    endif()
endif()

########################################################
# Header availabilities
# This list is from config/PETSc/Configure.py
list(APPEND SEARCHHEADERS setjmp dos endian fcntl float io limits malloc
    pwd search strings unistd sys/sysinfo machine/endian sys/param sys/procfs sys/resource
    sys/systeminfo sys/times sys/utsname string stdlib sys/socket sys/wait netinet/in
    netdb Direct time Ws2tcpip sys/types WindowsX cxxabi float ieeefp stdint sched pthread mathimf
    signal dlfcn linux_header math sys/time fenv)
if (WIN32)
    list(APPEND SEARCHHEADERS Winsock2 Windows)
endif()
set(PETSCCONF_HAVE_HEADERS )
foreach(hdr ${SEARCHHEADERS})
    STRING(TOUPPER ${hdr} HDR)
    STRING(REPLACE "/" "_" HDR ${HDR})
    set(VARNAME "PETSC_HAVE_${HDR}_H")
    CHECK_INCLUDE_FILES("${hdr}.h" ${VARNAME})
    if (${${VARNAME}})
        list(APPEND PETSCCONF_HAVE_HEADERS "#define ${VARNAME} 1")
    endif()
endforeach()
STRING(REPLACE ";" "\n\n" PETSCCONF_HAVE_HEADERS "${PETSCCONF_HAVE_HEADERS}")
#message(STATUS "Detected available headers: ${PETSCCONF_HAVE_HEADERS}")

# Without sys/socket.h no socket viewer
# Maybe there's a more advanced logic behind that, but this is troublesome on windows only (until now)
set(PETSC_USE_SOCKET_VIEWER ${PETSC_HAVE_SYS_SOCKET_H})

# __SSE__
CHECK_SYMBOL_EXISTS(__SSE__ "" PETSC_HAVE_SSE)

if (WIN32)
    # check availability of uid_t and gid_t
    if (PETSC_HAVE_UNISTD_H)
        CHECK_SYMBOL_EXISTS(uid_t "unistd.h" PETSC_HAVE_UID_T)
        CHECK_SYMBOL_EXISTS(gid_t "unistd.h" PETSC_HAVE_GID_T)
    elseif (PETSC_HAVE_SYS_TYPES_H)
        CHECK_SYMBOL_EXISTS(uid_t "sys/types.h" PETSC_HAVE_UID_T)
        CHECK_SYMBOL_EXISTS(gid_t "sys/types.h" PETSC_HAVE_GID_T)
    endif()
    
    # __SSE__
    if (NOT PETSC_HAVE_SSE)
        set(PETSC_HAVE_XMMINTRIN_H NO)
    endif()
    
    
endif()

########################################################
# Signal availabilities
set(SEARCHSIGNALS ABRT ALRM BUS CHLD CONT FPE HUP ILL INT KILL PIPE QUIT SEGV
    STOP SYS TERM TRAP TSTP URG USR1 USR2)
set(PETSCCONF_HAVE_SIGNAL )
foreach(sig ${SEARCHSIGNALS})
    if (PETSC_HAVE_SIGNAL_H)
        CHECK_SYMBOL_EXISTS("SIG${sig}" "signal.h" PETSC_HAVE_SIG${sig})
    endif()
    if (NOT PETSC_HAVE_SIG${sig})
        list(APPEND PETSCCONF_HAVE_SIGNAL "#define PETSC_MISSING_SIG${sig} 1")
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
list(APPEND SEARCHFUNCTIONS access _access clock drand48 getcwd _getcwd getdomainname gethostname
    gettimeofday getwd memalign memmove mkstemp popen PXFGETARG rand getpagesize
    readlink realpath sigaction signal sigset usleep sleep _sleep socket times
    gethostbyname uname snprintf _snprintf lseek _lseek time fork stricmp 
    strcasecmp bzero dlopen dlsym dlclose dlerror get_nprocs sysctlbyname _set_output_format
    closesocket WSAGetLastError # from missing.py:65 / socket stuff
    # Added in Petsc 3.6.1 (at least here)
    vsnprintf va_copy getrusage vfprintf nanosleep sysinfo vprintf
    # Added in the verge of fixing up for VS2013
    get_command_argument getarg PXFGETARG
)
if (WIN32)
    list(APPEND SEARCHFUNCTIONS GetComputerName)
endif()
set(PETSCCONF_HAVE_FUNCS )
foreach(func ${SEARCHFUNCTIONS})
    STRING(TOUPPER ${func} FUNC)
    set(VARNAME "PETSC_HAVE_${FUNC}")
    CHECK_FUNCTION_EXISTS(${func} ${VARNAME})
    if (${${VARNAME}})
        list(APPEND PETSCCONF_HAVE_FUNCS "#define ${VARNAME} 1")
    endif()
endforeach()
# MPI extras - see Buildsystem/config/packages/MPI.py lines 781 ff
if (PETSC_HAVE_MPI)
    if (PETSC_HAVE_MPI_WIN_CREATE)
        set(PETSC_HAVE_MPI_REPLACE 1)
    endif()
    if (NOT PETSC_HAVE_MPI_TYPE_CREATE_INDEXED_BLOCK)
        set(PETSC_HAVE_MPI_ALLTOALLW NO)
    endif()
    CHECK_FUNCTION_EXISTS(MPIDI_CH3I_sock_set PETSC_HAVE_MPICH_CH3_SOCK)
    CHECK_FUNCTION_EXISTS(MPIDI_CH3I_sock_fixed_nbc_progress PETSC_HAVE_MPICH_CH3_SOCK_FIXED_NBC_PROGRESS)
    CHECK_SYMBOL_EXISTS(MPI_Comm_f2c mpi.h PETSC_HAVE_MPI_COMM_F2C)
    CHECK_SYMBOL_EXISTS(MPI_Comm_c2f mpi.h PETSC_HAVE_MPI_COMM_C2F)
    CHECK_SYMBOL_EXISTS(MPI_IN_PLACE mpi.h PETSC_HAVE_MPI_IN_PLACE)
    
    # The CheckSymbolExists wont find enum types - see http://www.cmake.org/cmake/help/v3.2/module/CheckSymbolExists.html
    #CHECK_SYMBOL_EXISTS(MPI_COMBINER_DUP mpi.h PETSC_HAVE_MPI_COMBINER_DUP)
    trycompile(PETSC_HAVE_MPI_COMBINER_DUP "#include <mpi.h>" "int combiner = MPI_COMBINER_DUP;" c)
    #CHECK_SYMBOL_EXISTS(MPI_COMBINER_CONTIGUOUS mpi.h PETSC_HAVE_MPI_COMBINER_CONTIGUOUS)
    trycompile(PETSC_HAVE_MPI_COMBINER_CONTIGUOUS "#include <mpi.h>" "int combiner = MPI_COMBINER_CONTIGUOUS;" c)
    # MPI_Fint is a typedef - - see http://www.cmake.org/cmake/help/v3.2/module/CheckSymbolExists.html
    #CHECK_SYMBOL_EXISTS(MPI_Fint mpi.h PETSC_HAVE_MPI_FINT)
    trycompile(PETSC_HAVE_MPI_FINT "#include <mpi.h>" "MPI_Fint a;" c)
    
    # Data types
    foreach(MPI_DATATYPE MPI_LONG_DOUBLE MPI_INT64_T MPI_C_DOUBLE_COMPLEX)
        CHECK_SYMBOL_EXISTS(${MPI_DATATYPE} mpi.h PETSC_HAVE_${MPI_DATATYPE})
        if (PETSC_HAVE_${MPI_DATATYPE})
            list(APPEND PETSCCONF_HAVE_FUNCS "#define PETSC_HAVE_${MPI_DATATYPE} 1")
        endif()
    endforeach()
    
    # This was originally "MINGW OR WIN32" - The FindMPI wrapper from OpenCMISS manage however
    # now defines MPI_Fortran_MODULE_COMPATIBLE if the "USE MPI" directive works.
    if (MPI_Fortran_MODULE_COMPATIBLE)
        set(PETSC_HAVE_MPI_F90MODULE YES)
    endif()
endif()
STRING(REPLACE ";" "\n\n" PETSCCONF_HAVE_FUNCS "${PETSCCONF_HAVE_FUNCS}")
#message(STATUS "Detected available functions: ${PETSCCONF_HAVE_FUNCS}")

# MPI-IO      
set(_MPIIO_CODELIST "MPI_Aint lb, extent\;\nif (MPI_Type_get_extent(MPI_INT, &lb, &extent))\;\n"
    "MPI_File fh\;\nvoid *buf\;\nMPI_Status status\;\nif (MPI_File_write_all(fh, buf, 1, MPI_INT, &status))\;\n"
    "MPI_File fh\;\nvoid *buf\;\nMPI_Status status\;\nif (MPI_File_read_all(fh, buf, 1, MPI_INT, &status))\;\n"
    "MPI_File fh\;\nMPI_Offset disp\;\nMPI_Info info\;\nif (MPI_File_set_view(fh, disp, MPI_INT, MPI_INT, \"\", info))\;\n"
    "MPI_File fh\;\nMPI_Info info\;\nif (MPI_File_open(MPI_COMM_SELF, \"\", 0, info, &fh))\;\n"
    "MPI_File fh\;\nMPI_Info info\;\nif (MPI_File_close(&fh))\;\n")
set(PETSC_HAVE_MPIIO YES)
foreach(_IDX RANGE 0 5)
    list(GET _MPIIO_CODELIST ${_IDX} _MPIIOCODE)
    trycompile(MPIIOCHECK${_IDX} "#include <mpi.h>" "${_MPIIOCODE}" c)
    if (NOT MPIIOCHECK${_IDX})
        set(PETSC_HAVE_MPIIO NO)
    endif()
endforeach()

########################################################
# Fortran interfacing - option above [and passed in as def by opencmiss]
if (FORTRAN_MANGLING STREQUAL Add_)
    set(PETSC_HAVE_FORTRAN_UNDERSCORE YES)
    set(PETSC_BLASLAPACK_UNDERSCORE YES)
elseif(FORTRAN_MANGLING STREQUAL UpCase)
    set(PETSC_HAVE_FORTRAN_CAPS YES)
    set(PETSC_BLASLAPACK_CAPS YES)
endif()
# Note: Not used anywhere but similar: PETSC_HAVE__GFORTRAN_IARGC (underscores!!)
CHECK_FUNCTION_EXISTS(_gfortran_iargc PETSC_HAVE_GFORTRAN_IARGC)
CHECK_FORTRAN_FUNCTION_EXISTS(get_command_argument PETSC_HAVE_FORTRAN_GET_COMMAND_ARGUMENT)
CHECK_FORTRAN_FUNCTION_EXISTS(getarg PETSC_HAVE_FORTRAN_GETARG)
CHECK_FUNCTION_EXISTS(ipxfargc_ PETSC_HAVE_PXFGETARG_NEW)
CHECK_FUNCTION_EXISTS(f90_unix_MP_iargc PETSC_HAVE_NAGF90)
CHECK_FUNCTION_EXISTS(iargc_ PETSC_HAVE_BGL_IARGC)
CHECK_FUNCTION_EXISTS("GETARG@16" PETSC_IARG_COUNT_PROGNAME)
if (PETSC_IARG_COUNT_PROGNAME)
    set(PETSC_USE_NARGS TRUE)
endif()

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
set(PETSC_DEPENDENCIES PASTIX MUMPS SUITESPARSE SCALAPACK PTSCOTCH
    SUPERLU SUNDIALS HYPRE SUPERLU_DIST PARMETIS)
set(PETSCCONF_HAVE_FLAGS )
set(PETSC_CONFIGINFO_STRING )
foreach(PACKAGE ${PETSC_DEPENDENCIES})
    # Define the option
    option(USE_${PACKAGE} "Build PETSc with ${PACKAGE}" ON)
    
    # See if we want to use it
    if (USE_${PACKAGE})
        message(STATUS "Building PETSC with ${PACKAGE}...")
        find_package(${PACKAGE} ${${PACKAGE}_VERSION} REQUIRED)
        STRING(TOLOWER ${PACKAGE} pkgname)
        
        # Set the petsc-have flag
        set(PETSC_HAVE_${PACKAGE} YES)
        
        # Add targets to link targets list
        list(APPEND PETSC_PACKAGE_LIBS ${pkgname})
        
        # petscconfig.h
        list(APPEND PETSCCONF_HAVE_FLAGS "#ifndef PETSC_HAVE_${PACKAGE}\n#define PETSC_HAVE_${PACKAGE} 1\n#endif\n\n")
        
        # petscconfiginfo.h
        set(INCLUDES )
        set(LIBRARIES )
        get_target_property(INCDIR ${pkgname} INTERFACE_INCLUDE_DIRECTORIES)
        list(APPEND INCLUDES ${INCDIR})
        get_target_property(LINK_LIBS ${pkgname} INTERFACE_LINK_LIBRARIES)
        list(APPEND LIBRARIES ${LINK_LIBS})
        STRING(REPLACE ";" "," LIBRARIES "${LIBRARIES}")
        STRING(REPLACE ";" "," INCLUDES "${INCLUDES}")
        list(APPEND PETSC_CONFIGINFO_STRING "--with-${pkgname}=1 --with-${pkgname}-lib=[${LIBRARIES}] --with-${pkgname}-include=[${INCLUDES}]")
    endif()
endforeach()

########################################################
# X11
find_package(X11 QUIET)
if (X11_FOUND)
    set(PETSC_HAVE_X TRUE)
    list(APPEND PETSC_PACKAGE_LIBS ${X11_LIBRARIES})
    list(APPEND PETSC_PACKAGE_INCLUDES ${X11_INCLUDE_DIR})
endif()

########################################################
# SSL support
find_package(OpenSSL QUIET)
if (OPENSSL_FOUND AND NOT MINGW)
    message(STATUS "Building PETSc with OpenSSL ${OPENSSL_VERSION}")
    list(APPEND PETSC_PACKAGE_LIBS ${OPENSSL_LIBRARIES})
    set(PETSC_HAVE_SSL 1)
endif()

########################################################
# OpenMP
if (WITH_OPENMP)
  find_package(OpenMP)
  if (OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${OpenMP_Fortran_FLAGS}")
    set(PETSC_HAVE_OPENMP 1)
  else()
    set(WITH_OPENMP FALSE)
    set(PETSC_HAVE_OPENMP FALSE)
  endif()
endif()

########################################################
### __FUNCT__ debug stuff
include(CheckCSourceCompiles)
CHECK_C_SOURCE_COMPILES("
#include <stdio.h>
int main(void) {
    printf(\"%s\", __func__);
    return 0;
}" HAVE_COMPILER__func__)

CHECK_C_SOURCE_COMPILES("
#include <stdio.h>
int main(void) {
    printf(\"%s\", __FUNCTION__);
    return 0;
}" HAVE_COMPILER__FUNCTION__)

set(PETSC_FUNCTION_NAME_C __FUNCT__)
set(PETSC_FUNCTION_NAME_CXX __FUNCT__)
if(HAVE_COMPILER__func__)
    set(PETSC_FUNCTION_NAME_C __func__)
    set(PETSC_FUNCTION_NAME_CXX __func__)
elseif(HAVE_COMPILER__FUNCTION__)
    set(PETSC_FUNCTION_NAME_C __FUNCTION__)
    set(PETSC_FUNCTION_NAME_CXX __FUNCTION__)
endif()

########################################################
# Look for restrict support
set(RESTRICT_CODE
    "int main(void)
     {
         int* RESTRICT a;
         return 0;
     }")
set(RESTRICT_KEYWORD )     
SET(restrict_keywords __restrict__ __restrict restrict)
foreach(restrict_keyword ${restrict_keywords})
    set(CMAKE_REQUIRED_DEFINITIONS "-DRESTRICT=${restrict_keyword}")
    check_c_source_compiles("${RESTRICT_CODE}" HAVE_${restrict_keyword})
    if (HAVE_${restrict_keyword})
        set(RESTRICT_KEYWORD ${restrict_keyword})
        break()
    endif()
endforeach()
set(PETSC_C_RESTRICT ${RESTRICT_KEYWORD})
set(PETSC_CXX_RESTRICT ${RESTRICT_KEYWORD})

#########################################
# Check for complex numbers in in C99 std
# Note that since PETSc source code uses _Complex we test specifically for that, not complex
CHECK_C_SOURCE_COMPILES("
#include <complex.h>
int main(void) {
    double _Complex x;
    x = I;
    return 0;
}" PETSC_HAVE_C99_COMPLEX)

#########################################
# Misc C stuff
CHECK_C_SOURCE_COMPILES("
int main(void) {
    if (__builtin_expect(0,1)) return 1;
}" PETSC_HAVE_BUILTIN_EXPECT)
      
#########################################
# Windows-specific stuff

CHECK_C_SOURCE_COMPILES("
#include <sys/stat.h>
int main(void) {
    int a=0;
    if (S_ISDIR(a)){};
    return 0;
}" PETSC_HAVE_S_ISDIR)

#if self.libraries.add('Kernel32.lib','GetComputerName',prototype='#include <Windows.h>', call='GetComputerName(NULL,NULL);'):
#      self.addDefine('HAVE_WINDOWS_H',1)
#      self.addDefine('HAVE_GETCOMPUTERNAME',1)
#      kernel32=1
#    elif self.libraries.add('kernel32','GetComputerName',prototype='#include <Windows.h>', call='GetComputerName(NULL,NULL);'):
#      self.addDefine('HAVE_WINDOWS_H',1)
#      self.addDefine('HAVE_GETCOMPUTERNAME',1)
#      kernel32=1
#    if kernel32:
#      if self.framework.argDB['with-windows-graphics']:
#        self.addDefine('USE_WINDOWS_GRAPHICS',1)
#      if self.checkLink('#include <Windows.h>','LoadLibrary(0)'):
#        self.addDefine('HAVE_LOADLIBRARY',1)
#      if self.checkLink('#include <Windows.h>','GetProcAddress(0,0)'):
#        self.addDefine('HAVE_GETPROCADDRESS',1)
#      if self.checkLink('#include <Windows.h>','FreeLibrary(0)'):
#        self.addDefine('HAVE_FREELIBRARY',1)
#      if self.checkLink('#include <Windows.h>','GetLastError()'):
#        self.addDefine('HAVE_GETLASTERROR',1)
#      if self.checkLink('#include <Windows.h>','SetLastError(0)'):
#        self.addDefine('HAVE_SETLASTERROR',1)
#      if self.checkLink('#include <Windows.h>\n','QueryPerformanceCounter(0);\n'):
#        self.addDefine('USE_MICROSOFT_TIME',1)
#    if self.libraries.add('Advapi32.lib','GetUserName',prototype='#include <Windows.h>', call='GetUserName(NULL,NULL);'):
#      self.addDefine('HAVE_GET_USER_NAME',1)
#    elif self.libraries.add('advapi32','GetUserName',prototype='#include <Windows.h>', call='GetUserName(NULL,NULL);'):
#      self.addDefine('HAVE_GET_USER_NAME',1)
#    if not self.libraries.add('User32.lib','GetDC',prototype='#include <Windows.h>',call='GetDC(0);'):
#      self.libraries.add('user32','GetDC',prototype='#include <Windows.h>',call='GetDC(0);')
#    if not self.libraries.add('Gdi32.lib','CreateCompatibleDC',prototype='#include <Windows.h>',call='CreateCompatibleDC(0);'):
#      self.libraries.add('gdi32','CreateCompatibleDC',prototype='#include <Windows.h>',call='CreateCompatibleDC(0);')
#    if self.checkCompile('#include <Windows.h>\n','LARGE_INTEGER a;\nDWORD b=a.u.HighPart;\n'):
#      self.addDefine('HAVE_LARGE_INTEGER_U',1)
    # Windows requires a Binary file creation flag when creating/opening binary files.  Is a better test in order?
#    if self.checkCompile('#include <Windows.h>\n#include <fcntl.h>\n', 'int flags = O_BINARY;'):
#      self.addDefine('HAVE_O_BINARY',1)
#    if self.compilers.CC.find('win32fe') >= 0:
#      self.addDefine('PATH_SEPARATOR','\';\'')
#      self.addDefine('DIR_SEPARATOR','\'\\\\\'')
#      self.addDefine('REPLACE_DIR_SEPARATOR','\'/\'')
#      self.addDefine('CANNOT_START_DEBUGGER',1)
#      (petscdir,error,status) = self.executeShellCommand('cygpath -w '+self.petscdir.dir)
#      self.addDefine('DIR','"'+petscdir.replace('\\','\\\\')+'"')
#    else:
#      self.addDefine('PATH_SEPARATOR','\':\'')
#      self.addDefine('REPLACE_DIR_SEPARATOR','\'\\\\\'')
#      self.addDefine('DIR_SEPARATOR','\'/\'')
#      self.addDefine('DIR', '"'+self.petscdir.dir+'"')

message(WARNING "THERE ARE STILL SOME SETTINGS THAT REQUIRE PROPER DETECTION!")
#PETSC_SIZEOF_MPI_COMM
#PETSC_SIZEOF_MPI_FINT
#PETSC_BITS_PER_BYTE
#PETSC_SIZEOF_VOID_P
#PETSC_SIZEOF_LONG
#PETSC_SIZEOF_SIZE_T
#PETSC_SIZEOF_INT
#PETSC_SIZEOF_LONG_LONG
#PETSC_SIZEOF_SHORT
#PETSC_C_RESTRICT
#PETSC_CXX_RESTRICT
