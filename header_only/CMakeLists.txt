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
set_target_properties( header_only PROPERTIES LINKER_LANGUAGE CXX INTERFACE_COMPILE_FEATURES cxx_std_23 )
add_library( cpu_info::header_only ALIAS header_only )

install( FILES cpu_info.hpp DESTINATION include )
install( TARGETS header_only EXPORT header_onlyTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
install( EXPORT header_onlyTargets FILE header_onlyTargets.cmake NAMESPACE cpu_info::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
# Buildtree-Export
export( EXPORT header_onlyTargets
	FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/header_onlyTargets.cmake
	NAMESPACE cpu_info::
)
