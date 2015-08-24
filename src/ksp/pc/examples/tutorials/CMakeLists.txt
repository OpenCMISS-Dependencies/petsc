add_executable(run_ksp_pc_tutorials_3 ex3.c)
target_link_libraries(run_ksp_pc_tutorials_3 petsc)
ADDTEST(ksp_pc_tutorials_3_np8 8 run_ksp_pc_tutorials_3 output/ex3_1.out "-n 100 -ksp_type cg -pc_type bjacobi -sub_pc_type icc -ksp_rtol 1.e-8 ")
ADDTEST(ksp_pc_tutorials_3_np8_v1 8 run_ksp_pc_tutorials_3 output/ex3_2.out "-n 100 -ksp_type preonly -pc_type redistribute -redistribute_ksp_type cg -redistribute_pc_type bjacobi -redistribute_sub_pc_type icc -redistribute_ksp_rtol 1.e-8 ")
