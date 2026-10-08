cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - cpu_topo
    ===================
    This component enhances cpu_info with the cpu_topo class, which can be used for
    thourough inspection of a system.
]]
add_library( cpu_topo INTERFACE cpu_topo.hpp )
set_target_properties( cpu_topo PROPERTIES
    LINKER_LANGUAGE CXX
    INTERFACE_COMPILE_FEATURES cxx_std_23
)
target_include_directories( cpu_topo INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CPU_INFO_COMPONENTS}/cpu_info>"
    "$<BUILD_INTERFACE:${CPU_INFO_COMPONENTS}/cpu_id>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cpu_topo>"
)
target_link_libraries( cpu_topo INTERFACE cpu_id )
add_library( cpu_info::cpu_topo ALIAS cpu_topo )
install( FILES cpu_topo.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpu_topo )
