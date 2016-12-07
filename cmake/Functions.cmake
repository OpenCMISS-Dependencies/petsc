include(CheckFunctionExists)
include(CheckFortranFunctionExists)
include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
include(CheckFortranSourceCompiles)
INCLUDE(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckTypeSize)

# Tries to compile given source code (of given type) and uses the currently set compilers (maybe MPI) 
function(trycompile VARIABLE INCLUDES CODE EXT)
    # This check makes sure the variable is not already in the cache
    #message(STATUS "Entering trycompile for ${VARIABLE}: ${${VARIABLE}}")
    if(NOT DEFINED "${VARIABLE}" OR "x${${VARIABLE}}" STREQUAL "x${VARIABLE}")
        #message(STATUS "Compiling code to check for ${VARIABLE}")
        #SET(BINDIR ${CMAKE_CURRENT_BINARY_DIR}/trycompile)
        #file(MAKE_DIRECTORY ${BINDIR})
        #STRING(RANDOM LENGTH 6 SALT)
        #SET(SOURCEFILE ${BINDIR}/trycompile_${VARIABLE}_${SALT}.${EXT})
        if ("${EXT}" STREQUAL "c" OR "${EXT}" STREQUAL "cpp")
            SET(STUB "
                ${INCLUDES}
                #ifdef __CLASSIC_C__
                int main(){
                    int argc;
                    char*argv[];
                #else
                int main(int argc, char *argv[]) {
                #endif
                    ${CODE}
                    if(argc > 1000) {
                        return *argv[0];
                    }
                    return 0;
                }
                "
                )
            CHECK_C_SOURCE_COMPILES("${STUB}" ${VARIABLE})
            #SET(COMPILER -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
            #if (${EXT} STREQUAL "cpp")
            #    SET(COMPILER -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
            #endif()
        elseif("${EXT}" STREQUAL "f" OR "${EXT}" STREQUAL "f90")
            SET(STUB "
                ${INCLUDES}
                program TESTFortran
                    ${CODE}
                end program TESTFortran
                "
                )
            #SET(COMPILER -DCMAKE_Fortran_COMPILER=${CMAKE_Fortran_COMPILER})
            CHECK_FORTRAN_SOURCE_COMPILES("${STUB}" ${VARIABLE})
        else()
            message(FATAL_ERROR "Not implemented: extension '${EXT}'")
        endif()
    endif()
endfunction()

# Convenience implementation for PETSC context
function(CHECK_TYPE_EXISTS NAME VARIABLE)
    set(CMAKE_EXTRA_INCLUDE_FILES ${ARGN})
    CHECK_TYPE_SIZE(${NAME} ${VARIABLE})
    set(CMAKE_EXTRA_INCLUDE_FILES)
    # The convention of CHECK_TYPE_SIZE is to set HAVE_<typename>. Here, we need PETSC_HAVE_<typename>
    set(PETSC_HAVE_${VARIABLE} NO PARENT_SCOPE)
    if (HAVE_${VARIABLE})
        set(PETSC_HAVE_${VARIABLE} YES PARENT_SCOPE)
        set(PETSC_SIZEOF_${VARIABLE} ${${VARIABLE}})
    endif()
endfunction()