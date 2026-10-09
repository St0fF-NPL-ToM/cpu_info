cmake_minimum_required( VERSION 4.2 )

set( DEBHLP cpu_info.natvis cpu_info.lldb )
target_sources( cpu_info INTERFACE FILE_SET 03_DEBUGGING TYPE HEADERS FILES ${DEBHLP} )
