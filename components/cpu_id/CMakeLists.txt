cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - cpu_id
    =================
    This component enhances cpu_info with the cpu_id class, which can be used for
    thourough inspection of a single core without having to be bound to it.

    How: cpu_id is the collection of all cpuid-leafs of the core. An instance that
    is not default-constructed - instead: using the explicit CTor - will populate
    upon instancing (bind current thread to the specific core, release afterwards).
]]
add_library( cpu_id INTERFACE cpu_id.hpp )
set_target_properties( cpu_id PROPERTIES
    LINKER_LANGUAGE CXX
    INTERFACE_COMPILE_FEATURES cxx_std_23
)
target_include_directories( cpu_id INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CPU_INFO_COMPONENTS}/cpu_info>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cpu_id>"
)
target_link_libraries( cpu_id INTERFACE cpu_info )
add_library( cpu_info::cpu_id ALIAS cpu_id )
install( FILES cpu_id.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpu_id )
