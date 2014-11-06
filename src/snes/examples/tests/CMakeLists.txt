macro(ADDEXEC name)
    add_executable(${name} ${name}${ARGN})
    target_link_libraries(${name} petsc)
endmacro()

macro(ADDTEST name exec np)  
    add_test(${name} ${CMAKE_COMMAND}
      -DCMD=${MPIEXEC}
      -DARGS="${MPIEXEC_NUMPROC_FLAG} ${np} ${CMAKE_CURRENT_BINARY_DIR}/${exec} ${ARGN}"
      -DTARGET=${CMAKE_CURRENT_SOURCE_DIR}/output/${name}.out
      -P ${PETSc_SOURCE_DIR}/testhelper.cmake)
endmacro()

ADDEXEC(ex1)
ADDTEST(ex1_1 ex1 1 -ksp_gmres_cgs_refinement_type refine_always)
ADDTEST(ex1_2 ex1 1 -snes_monitor_short -snes_type newtontr -ksp_gmres_cgs_refinement_type refine_always)
ADDTEST(ex1_3 ex1 1 -snes_monitor_short -mat_coloring_type sl  -snes_fd_coloring -mx 8 -my 11 -ksp_gmres_cgs_refinement_type refine_always)

ADDEXEC(ex1f .F)
ADDTEST(ex1f_1 ex1f 1 -snes_monitor_short -nox -snes_type newtontr -ksp_gmres_cgs_refinement_type refine_always)
ADDTEST(ex1f_2 ex1f 1 -snes_monitor_short -nox -snes_fd -ksp_gmres_cgs_refinement_type refine_always)
ADDTEST(ex1f_3 ex1f 1 -snes_monitor_short -nox -snes_fd_coloring -ksp_gmres_cgs_refinement_type refine_always)

ADDEXEC(ex5)
# no output files available to compare!

ADDEXEC(ex7)
ADDTEST(ex7_1 ex7 1 -ksp_gmres_cgs_refinement_type refine_always -snes_monitor_cancel -snes_monitor_short)
ADDTEST(ex7_1 ex7 1 -variant -ksp_gmres_cgs_refinement_type refine_always -snes_monitor_cancel -snes_monitor_short)
#ADDEXEC(ex8) # ref to DMDAGetIerr undefined
#ADDEXEC(ex15) #../src/snes/impls/vi/viimpl.h is missing
#ADDEXEC(ex16) # ref to DMDAGetIerr undefined

ADDEXEC(ex17)
ADDTEST(ex17_1 ex17 1 -snes_monitor_short)

ADDEXEC(ex68)
# no output files available to compare!

ADDEXEC(ex12f .F)
ADDTEST(ex12f_1 ex12f 2 -ksp_gmres_cgs_refinement_type refine_always -n 10 -snes_monitor_short)

ADDEXEC(ex14f .F)
ADDTEST(ex14f_1 ex14f 1 -snes_monitor_short)