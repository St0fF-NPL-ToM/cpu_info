cmake_minimum_required( VERSION 4.2 )

add_executable( cpu_info_example test_info.cpp )
target_link_libraries( cpu_info_example PUBLIC cpu_info )

add_executable( cpu_id_example test_id.cpp )
target_link_libraries( cpu_id_example PUBLIC cpu_id )

add_executable( cpu_topo_example test_topo.cpp )
target_link_libraries( cpu_topo_example PUBLIC cpu_topo )
