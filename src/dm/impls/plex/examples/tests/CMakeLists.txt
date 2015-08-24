add_executable(run_dm_impls_plex_tests_3 ex3.c)
target_link_libraries(run_dm_impls_plex_tests_3 petsc)
ADDTEST(dm_impls_plex_tests_3_np4_nonconforming_tensor_2 4 run_dm_impls_plex_tests_3 output/ex3_nonconforming_tensor_2.out "-petscpartitioner_type simple -tree -simplex 0 -dim 2 -num_comp 2 -dm_plex_max_projection_height 1 -petscspace_poly_tensor -petscspace_order 1 -qorder 1 -mat_null_space_test_view -dm_view ascii::ASCII_INFO_DETAIL ")
ADDTEST(dm_impls_plex_tests_3_np4_nonconforming_tensor_3 4 run_dm_impls_plex_tests_3 output/ex3_nonconforming_tensor_3.out "-petscpartitioner_type simple -tree -simplex 0 -dim 3 -num_comp 3 -dm_plex_max_projection_height 2 -petscspace_poly_tensor -petscspace_order 1 -qorder 1 -mat_null_space_test_view -dm_view ascii::ASCII_INFO_DETAIL ")
