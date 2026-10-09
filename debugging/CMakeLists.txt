cmake_minimum_required( VERSION 4.2 )

if ( WIN32 )
    set( DEBHLP cpu_info.natvis )
else()
    set( DEBHLP cpu_info.lldb )
endif()
target_sources( cpu_info INTERFACE FILE_SET 03_DEBUGGING TYPE HEADERS FILES ${DEBHLP} )
