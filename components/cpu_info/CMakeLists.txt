cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - header only library base
    ===================================
    Simply creates an import library from cpu_info.hpp …
]]
add_library( cpu_info INTERFACE cpu_info.hpp )
set_target_properties( cpu_info PROPERTIES
    LINKER_LANGUAGE CXX
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED on
    INTERFACE_COMPILE_FEATURES cxx_std_23
)
target_include_directories( cpu_info INTERFACE
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cpu_info>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
add_library( cpu_info::cpu_info ALIAS cpu_info )
install( FILES cpu_info.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpu_info )
