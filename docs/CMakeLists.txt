cmake_minimum_required( VERSION 4.2 )
#[[ cpu_info - documentation folder - just glob'em, the content will change over time ]]
file( GLOB_RECURSE DOCS LIST_DIRECTORIES off RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} * )
list( REMOVE_ITEM DOCS "CMakeLists.txt" )

target_sources( cpu_info PRIVATE 
	FILE_SET 04_DOCUMENTATION TYPE HEADERS
	BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/.."
	FILES ${DOCS}
)
