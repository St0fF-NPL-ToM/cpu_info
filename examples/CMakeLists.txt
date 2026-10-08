cmake_minimum_required( VERSION 4.2 )

add_executable( cpu_info_example test_info.cpp )
target_link_libraries( cpu_info_example PUBLIC cpu_info )
