cmake_minimum_required( VERSION 4.2 )

set( SOURCES cpu_info.cpp cpu_id.cpp )
set( HEADERS cpu_info.h cpu_id.h cpu_enums_intel.h cpu_set.hpp cpu_info_types.hpp )
set( DOCS cpu_info.natvis )

add_library( cpu_info ${HEADERS} ${SOURCES} ${DOCS} )
# target_compile_options( cpu_info PUBLIC "-Wall" )

source_group( DOCS FILES ${DOCS} )
source_group( HDRS FILES ${HEADERS} )
source_group( SRCS FILES ${SOURCES} )

configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cpu_info.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	VERSION "${PROJECT_VERSION}" COMPATIBILITY AnyNewerVersion )
install( FILES ${HEADERS} DESTINATION include )
install( FILES
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/cpu_infoConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpu_info )
