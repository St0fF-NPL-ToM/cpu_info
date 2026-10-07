cmake_minimum_required( VERSION 4.2 )

project( cpu_info VERSION 0.0.4 LANGUAGES CXX )
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

# create the library
add_subdirectory( source )
add_subdirectory( header_only )
add_subdirectory( docs )
add_subdirectory( debugging )

set( DOCS README.md LICENSE CONTRIBUTING.md CODE_OF_CONDUCT.md )
target_sources( cpu_info PRIVATE ${DOCS} )

option( cpu_info_example "build the example(s)?" off )
if ( cpu_info_example )
    add_subdirectory( example )
endif()

# library installation
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cpu_info.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info
	NO_CHECK_REQUIRED_COMPONENTS_MACRO )
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion )
install( FILES
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )

install( TARGETS cpu_info header_only EXPORT cpu_infoTargets
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
