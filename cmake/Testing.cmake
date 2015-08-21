enable_testing()
set(GENERATED_TESTS_INCFILE ${CMAKE_CURRENT_BINARY_DIR}/IncludeTestFolders.cmake)

if (NOT EXISTS "${GENERATED_TESTS_INCFILE}" OR TRUE)
    find_package(PythonInterp QUIET)
    if (PYTHONINTERP_FOUND)
        message(STATUS "Running python test case generator...")
        execute_process(COMMAND ${PYTHON_EXECUTABLE} 
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/convert_tests.py ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE RES
            ERROR_FILE ${CMAKE_CURRENT_BINARY_DIR}/conversion.error
            OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/conversion.out
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
         if (NOT RES EQUAL 0)
             message(WARNING "Python script for test case generation failed. See ${CMAKE_CURRENT_BINARY_DIR}/conversion.error for details.")
         endif()
    else()
        message(WARNING "No Python interpreter found. Cannot automatically generate test case scripts. No tests will be available.")
    endif()
endif()
            
if (EXISTS "${GENERATED_TESTS_INCFILE}")
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
        # Set to 10s timeout
        set_tests_properties(${NAME} PROPERTIES TIMEOUT 10)
    endmacro()
    
    include("${GENERATED_TESTS_INCFILE}")
endif()