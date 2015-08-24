add_executable(run_vec_is_tests_1 ex1.c)
target_link_libraries(run_vec_is_tests_1 petsc)
ADDTEST(vec_is_tests_1_np1_1 1 run_vec_is_tests_1 output/ex1_1.out "")
