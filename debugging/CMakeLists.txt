cmake_minimum_required( VERSION 4.2 )

if ( WIN32 )
    set( DEBHLP cpu_info.natvis )
else()
    set( DEBHLP )
endif()
target_sources( cpu_info PRIVATE ${DEBHLP} )
source_group( DOCS FILES ${DEBHLP} )
