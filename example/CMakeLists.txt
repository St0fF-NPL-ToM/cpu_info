cmake_minimum_required( VERSION 4.2 )

add_executable( cpu_info_example test.cpp )
target_link_libraries( cpu_info_example PRIVATE cpu_info::cpu_info )

add_executable( header_only_example test_ho.cpp )
target_link_libraries( header_only_example cpu_info::header_only )
