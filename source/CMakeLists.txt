cmake_minimum_required( VERSION 4.2 )
#[[
	cpu_info - "old style library" part
	===================================
	The following code produces the "old" cpu_info library target, even with in-tree support.
	The installation / packaging is only prepared.
]]
set( SOURCES cpu_info.cpp cpu_id.cpp )
set( HEADERS cpu_info.h cpu_id.h cpu_info_types.hpp cpu_enums_intel.h cpu_set.hpp )

add_library( cpu_info ${HEADERS} ${SOURCES} )
target_include_directories( cpu_info PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
add_library( cpu_info::cpu_info ALIAS cpu_info )

install( FILES ${HEADERS} DESTINATION include )
install( TARGETS cpu_info EXPORT cpu_infoTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
install( EXPORT cpu_infoTargets FILE cpu_infoTargets.cmake NAMESPACE cpu_info::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
# Buildtree-Export
export( EXPORT cpu_infoTargets
	FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpu_infoTargets.cmake
	NAMESPACE cpu_info::
)

source_group( HDRS FILES ${HEADERS} )
source_group( SRCS FILES ${SOURCES} )
source_group( DOCS REGULAR_EXPRESSION ".*([.](m?md)|(txt))|(LICENSE)" )
source_group( DBG_HLP REGULAR_EXPRESSION ".*[.]((natvis)|(lldb)|(py))" )
