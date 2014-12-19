include(CheckFunctionExists)
include(CheckFortranFunctionExists)
include(CheckCSourceCompiles)
include(CheckFortranSourceCompiles)
INCLUDE(CheckIncludeFiles)

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
        if (${EXT} STREQUAL "c" OR ${EXT} STREQUAL "cpp")
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
        elseif(${EXT} STREQUAL "f" OR ${EXT} STREQUAL "f90")
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
#        file(WRITE ${SOURCEFILE} "${STUB}")
#        configure_file(${SOURCEFILE} ${SOURCEFILE} @ONLY)
#        try_compile(${VARIABLE} ${BINDIR}
#                SOURCES ${SOURCEFILE}
#                CMAKE_FLAGS ${COMPILER}
#                ${ARGN}
#                OUTPUT_VARIABLE OUTPUT)
        #set(${VARIABLE} ${${VARIABLE}} CACHE INTERNAL "Try compile test for ${VARIABLE}")
#        set(${VARIABLE} ${${VARIABLE}} PARENT_SCOPE)
#        message(STATUS "Compiling code to check for ${VARIABLE} .. ${${VARIABLE}}")
#        if (NOT ${VARIABLE})
#            file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
#                "Determining if code for variable ${VARIABLE} failed with the following output:\n"
#                "${OUTPUT}\n\n")
#        endif()
        #file(REMOVE ${SOURCEFILE})
    endif()
endfunction()


function(checkexists VARIABLE FUNC)
    #message(STATUS "Checking if ${FUNC} exists")
    if (${ARGC} GREATER 0 AND NOT "${ARGN}" STREQUAL "")
        #message(STATUS "Looking in extra libraries ${ARGN}")
        SET(CMAKE_REQUIRED_LIBRARIES ${ARGN})
    endif()
    #SET(CMAKE_REQUIRED_DEFINITIONS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
    CHECK_FUNCTION_EXISTS(${FUNC} ${VARIABLE})
    set(${VARIABLE} ${${VARIABLE}} PARENT_SCOPE)
    #message(STATUS "Checking if ${FUNC} exists .. ${RESULT_VAR}")
endfunction()

function(checkfexists VARIABLE FUNC)
    #message(STATUS "Checking if Fortran ${FUNC} exists")
    if (${ARGC} GREATER 0 AND NOT ${ARGN} STREQUAL "")
        #message(STATUS "Looking in extra libraries ${ARGN}")
        SET(CMAKE_REQUIRED_LIBRARIES ${ARGN})
    endif()
    SET(CMAKE_REQUIRED_DEFINITIONS -DCMAKE_Fortran_COMPILER=${CMAKE_Fortran_COMPILER})
    CHECK_FORTRAN_FUNCTION_EXISTS(${FUNC} ${VARIABLE})
    set(${VARIABLE} ${${VARIABLE}} PARENT_SCOPE)
    #message(STATUS "Checking if ${FUNC} exists .. ${RESULT_VAR}")
endfunction()