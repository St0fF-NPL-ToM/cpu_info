cmake_minimum_required( VERSION 4.2 )
#[[
    cpu_info - component: cmake

    library installation preparation:
    - package config file
    - package version file
    - installation of both
]]
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
