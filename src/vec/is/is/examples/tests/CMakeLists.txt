add_executable(run_vec_is_is_tests_6 ex6.c)
target_link_libraries(run_vec_is_is_tests_6 petsc)
ADDTEST(vec_is_is_tests_6_np3_3 3 run_vec_is_is_tests_6 output/ex6_3.out "")
