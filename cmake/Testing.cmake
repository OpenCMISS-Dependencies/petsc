enable_testing()
set(TESTRUNNER ${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestRunner.cmake)
macro(ADDTEST NAME NPROC TARGET ARGS)
    add_test(NAME ${NAME}
     COMMAND ${CMAKE_COMMAND} 
    -DMPIEXEC=${MPIEXEC}
    #-DMPIEXEC=$<TARGET_FILE:helper>
    -DNPFLAG:STRING=${MPIEXEC_NUMPROC_FLAG}
    -DNPROC=${NPROC}
    -DEXEC=$<TARGET_FILE:${TARGET}>
    -DARGS:STRING=${ARGS}
    -DOUT=${NAME}.tmp
    -DWD=${CMAKE_CURRENT_BINARY_DIR}
    -P ${TESTRUNNER}
    )
endmacro()

add_subdirectory(./src/vec/vec/examples/tests)
#add_subdirectory(./src/vec/is/is/examples/tests)
#add_subdirectory(./src/vec/is/examples/tests)
#add_subdirectory(./src/vec/is/ao/examples/tests)
#add_subdirectory(./src/ksp/ksp/examples/tests)
#add_subdirectory(./src/ksp/pc/examples/tests)
#add_subdirectory(./src/snes/examples/tests)
#add_subdirectory(./src/mat/examples/tests)
#add_subdirectory(./src/tao/unconstrained/examples/tests)
#add_subdirectory(./src/dm/impls/plex/examples/tests)
#add_subdirectory(./src/dm/impls/moab/examples/tests)
#add_subdirectory(./src/dm/impls/patch/examples/tests)
#add_subdirectory(./src/dm/dt/examples/tests)
#add_subdirectory(./src/dm/examples/tests)
#add_subdirectory(./src/sys/error/examples/tests)
#add_subdirectory(./src/sys/examples/tests)
#add_subdirectory(./src/sys/classes/viewer/examples/tests)
#add_subdirectory(./src/sys/classes/draw/examples/tests)
#add_subdirectory(./src/ts/examples/tests)

