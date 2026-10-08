cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - components
    =====================
    This is where the library is being built from 3 components using
    a macro, so not everything is repeated …
]]
macro( cpu_lib name )
    add_library( ${name} INTERFACE ${name}.hpp )
    set_target_properties( ${name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED on
        LINKER_LANGUAGE CXX
        INTERFACE_COMPILE_FEATURES cxx_std_23
    )
    target_include_directories( ${name} SYSTEM INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CPU_INFO_INSTALL_INCLUDES}>"
    )
    add_library( cpu_info::${name} ALIAS ${name} )
    install( FILES ${name}.hpp DESTINATION ${CPU_INFO_INSTALL_INCLUDES} )
endmacro()
#[[
    cpu_info - header only library base
    ===================================
    Simply creates an import library from cpu_info.hpp …
]]
cpu_lib( cpu_info )
#[[
    cpu_info - cpu_id + cpu_topo
    ============================
]]
cpu_lib( cpu_id )
cpu_lib( cpu_topo )

#[[ finally: add cmake project file and debugging helpers ]]
add_subdirectory( cmake )
add_subdirectory( debugging )
