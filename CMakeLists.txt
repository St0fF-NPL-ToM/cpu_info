cmake_minimum_required( VERSION 4.2 )

project( cpu_info VERSION 0.0.2 LANGUAGES CXX )

include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

set( CMAKE_CXX_STANDARD 23 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# create the library
add_subdirectory( source )
add_subdirectory( docs )
add_subdirectory( debugging )

set( DOCS README.md LICENSE CONTRIBUTING.md CODE_OF_CONDUCT.md )
target_sources( cpu_info PRIVATE ${DOCS} )

add_library( cpu_info::cpu_info ALIAS cpu_info )
target_include_directories( cpu_info PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/source/>
    $<INSTALL_INTERFACE:include>
)

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

option( cpu_info_example "build the example program?" off )
if ( cpu_info_example )
    add_subdirectory( example )
endif()
