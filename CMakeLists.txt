cmake_minimum_required( VERSION 4.2 )

project( cpu_info VERSION 0.0.5 LANGUAGES CXX )
#[[
	cpu_info - the buildsystem.
	===========================
	- project setup, include helpers
	- subdirectories
	- append root-folder-files, so they're editable in an IDE
	- conditionally add example folder
]]
include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# prepare our library creation
set( CPU_INFO_COMPONENTS "${CMAKE_CURRENT_SOURCE_DIR}/components" )
set( CPU_INFO_INSTALL_INCLUDES "${CMAKE_INSTALL_INCLUDEDIR}/cpu_info" )

# create the library
add_subdirectory( components )
add_subdirectory( docs )

set( DOCS README.md LICENSE CONTRIBUTING.md CODE_OF_CONDUCT.md )
target_sources( cpu_info PRIVATE ${DOCS} )

option( CPUINFO_BUILD_EXAMPLES "build the examples?" off )
if ( CPUINFO_BUILD_EXAMPLES )
    add_subdirectory( examples )
endif()

install( TARGETS cpu_info cpu_id cpu_topo EXPORT cpu_infoTargets
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
