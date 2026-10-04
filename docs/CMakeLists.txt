cmake_minimum_required( VERSION 4.2 )

file( GLOB_RECURSE DOCS LIST_DIRECTORIES off RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.md *.mmd )

target_sources( cpu_info PRIVATE ${DOCS} )
source_group( DOCS FILES ${DOCS} )
