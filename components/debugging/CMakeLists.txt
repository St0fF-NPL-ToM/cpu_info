cmake_minimum_required( VERSION 4.2 )

if ( WIN32 )
    set( DEBHLP cpu_info.natvis )
else()
    set( DEBHLP cpu_info.lldb )
endif()
target_sources( cpu_info PRIVATE ${DEBHLP} )
