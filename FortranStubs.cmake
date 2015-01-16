find_package(SOWING ${SOWING_VERSION} CONFIG REQUIRED)
SET(bfort_opts -mnative -ansi -nomsgs -noprofile -anyname -mapptr -mpi -mpi2 -ferr
    -ptrprefix Petsc -ptr64 PETSC_USE_POINTER_CONVERSION -fcaps PETSC_HAVE_FORTRAN_CAPS 
    -fuscore PETSC_HAVE_FORTRAN_UNDERSCORE -f90mod_skip_header -f90modfile f90module.f90)
SET(EXCLUDE_DIRS SCCS output BitKeeper examples externalpackages bilinear
    ftn-auto fortran bin maint ftn-custom config f90-custom ftn-kernels
    finclude)
function(FTNGEN_RECURSE curdir)
    #message(STATUS "Entering ${curdir}")
    file(GLOB content RELATIVE ${curdir} ${curdir}/*)
    SET(sources )
    foreach(entry ${content})
        set(filename ${curdir}/${entry})
        if(IS_DIRECTORY ${filename})
            list(FIND EXCLUDE_DIRS ${entry} EXCLUDE_ME)
            if (EXCLUDE_ME EQUAL -1)
                FTNGEN_RECURSE(${filename})
            endif()
        else()
            get_filename_component(fext ${filename} EXT)
            # Process only c,h,cu files
            if (fext STREQUAL .c OR fext STREQUAL .h OR fext STREQUAL .cu)
                FTNGEN_PROCESS(${curdir} ${filename} ${fext})            
            endif()
        endif()
    endforeach()
endfunction()

function(FTNGEN_PROCESS DIRNAME SOURCE FEXT)
    SET(OUTDIR ${DIRNAME}/ftn-auto)
    if (NOT EXISTS ${OUTDIR})
        file(MAKE_DIRECTORY ${OUTDIR})
    endif() 
    get_filename_component(namewe ${filename} NAME_WE)
    SET(OUTFILE ${OUTDIR}/${namewe}f${FEXT})
    add_custom_command(OUTPUT ${OUTFILE}
        COMMAND bfort -dir ${OUTDIR} ${bfort_opts} ${SOURCE}
        COMMAND ${CMAKE_COMMAND} -DFILE=${OUTFILE} -P ${CMAKE_CURRENT_SOURCE_DIR}/FortranStubsHelper.cmake
    )
endfunction()

FTNGEN_RECURSE(${CMAKE_CURRENT_SOURCE_DIR})