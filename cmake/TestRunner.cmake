message(STATUS "Running equivalent of\n${MPIEXEC} ${NPFLAG} ${NPROC} ${EXEC} ${ARGS} > ${OUT}\nin folder ${WD}")
separate_arguments(ARGS)
#list(LENGTH ARGS LEN)
#message(STATUS "Arglen: ${LEN}, ARGS=${ARGS}")
execute_process(COMMAND ${MPIEXEC} ${NPFLAG} 
    ${NPROC} ${EXEC} ${ARGS}
    OUTPUT_FILE ${OUT}
    ERROR_FILE error.out
    WORKING_DIRECTORY ${WD})
file(READ error.out ERRMSG)
if (ERRMSG)
    message(FATAL_ERROR "Running test failed: ${ERRMSG}")
endif()