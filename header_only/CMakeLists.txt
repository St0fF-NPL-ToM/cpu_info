cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - header only library version
    ======================================
    This is the way easier variation.
    - add the header into the library
    - tell cmake what it needs to know
    - setup alias
    - set up how to install
]]
add_library( header_only INTERFACE cpu_info.hpp )
set_target_properties( header_only PROPERTIES
    LINKER_LANGUAGE CXX
    INTERFACE_COMPILE_FEATURES cxx_std_23
)
target_include_directories( header_only INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cpu_info_header_only>"
)
add_library( cpu_info::header_only ALIAS header_only )
install( FILES cpu_info.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpu_info_header_only )
