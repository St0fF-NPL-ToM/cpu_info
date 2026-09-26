cmake_minimum_required( VERSION 4.2 )

project( cpu_info VERSION 0.0.1 LANGUAGES CXX )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

add_library( cpu_info cpu_info.h cpu_info.cpp cpu_info.natvis cpu_id.h cpu_id.cpp  "cpu_enums.h")
add_library( cpu_info::cpu_info ALIAS cpu_info )
target_include_directories( cpu_info PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:include>
)

include( GNUInstallDirs )
include( CMakePackageConfigHelpers )
install( TARGETS cpu_info EXPORT cpu_infoTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
install( EXPORT cpu_infoTargets FILE cpu_infoTargets.cmake NAMESPACE cpu_info::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cpu_info.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion )
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/cpu_info.h DESTINATION include )
install( FILES
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
# Buildtree-Export
export( EXPORT cpu_infoTargets
	FILE ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpu_infoTargets.cmake
	NAMESPACE cpu_info::
)

option( cpu_info_example "build the example program?" off )
if ( cpu_info_example )
	add_executable( cpu_info_example test.cpp )
	target_link_libraries( cpu_info_example PRIVATE cpu_info::cpu_info )
endif()