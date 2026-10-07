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
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion )
install( FILES
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
