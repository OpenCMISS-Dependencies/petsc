message(STATUS "Running equivalent of\n${MPIEXEC} ${NPFLAG} ${NPROC} ${EXEC} ${ARGS} > ${OUT}\nin folder ${WD}")
separate_arguments(ARGS)
#list(LENGTH ARGS LEN)
#message(STATUS "Arglen: ${LEN}, ARGS=${ARGS}")
execute_process(COMMAND ${MPIEXEC} ${NPFLAG} 
    ${NPROC} ${EXEC} ${ARGS}
    OUTPUT_FILE ${OUT}
    ERROR_FILE error.out
    RESULT_VARIABLE EXEC_RETCODE
    WORKING_DIRECTORY ${WD})
set(CMP_RETCODE FALSE)
if (EXISTS "${COMPFILE}")
    message(STATUS "Comparing test output with ${COMPFILE}...")
    execute_process(COMMAND ${CMAKE_COMMAND} 
        -E compare_files ${OUT} ${COMPFILE}
        RESULT_VARIABLE CMP_RETCODE
        WORKING_DIRECTORY ${WD})
endif()

# Execution returned with an error -> boom
if (EXEC_RETCODE)
    file(READ error.out ERRMSG)
    message(FATAL_ERROR "Running test failed: ${ERRMSG}")
# Else: The output files are different. 
elseif(CMP_RETCODE)
    set(DIFFFILE ${CMAKE_CURRENT_BINARY_DIR}/test_diffs.log)
    message(STATUS "Test successfully executed but comparing output failed (might yet be okay!). Check diff log at ${DIFFFILE}")
    file(APPEND ${DIFFFILE} "-----------------------------------------------\n")
    file(APPEND ${DIFFFILE} "Test ${EXEC} finished but has different output:\n")
    find_program(DIFF diff)
    if (DIFF)
        execute_process(COMMAND ${DIFF} 
            ${OUT} ${COMPFILE}
            OUTPUT_VARIABLE DIFFOUT
            WORKING_DIRECTORY ${WD})
        file(APPEND ${DIFFFILE} ${DIFFOUT} "\n\n") 
    else()
        file(APPEND "No 'diff' program available. Giving full file contents. Test output:\n")
        file(READ ${OUT} _TMP)
        file(APPEND ${DIFFFILE} ${_TMP} "\n\n")
        file(APPEND "---------------------------\n'Ground truth' output:\n")
        file(READ ${COMPFILE} _TMP)
        file(APPEND ${DIFFFILE} ${_TMP} "\n\n")         
    endif()
endif()
# Some cleanup
file(REMOVE ${OUT})