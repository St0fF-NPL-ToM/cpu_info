cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - sourceFolder
    =======================
]]
set( SELF_CONTAINED_HEADERS 
    cpu_info.hpp
    cpu_id.hpp 
    cpu_topo.hpp 
)
add_library( cpu_info INTERFACE ${SELF_CONTAINED_HEADERS} )
set_target_properties( cpu_info PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED on
    LINKER_LANGUAGE CXX
    INTERFACE_COMPILE_FEATURES cxx_std_23
)
target_include_directories( cpu_info SYSTEM INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CPU_INFO_INSTALL_INCLUDES}>"
)
# create a FILE SET, which add_library cannot do somehow ...
target_sources( cpu_info INTERFACE
    FILE_SET 01_SOURCE_HEADERS TYPE HEADERS
    FILES ${SELF_CONTAINED_HEADERS}
)
# produce the namespace-contained alias
add_library( cpu_info::cpu_info ALIAS cpu_info )
