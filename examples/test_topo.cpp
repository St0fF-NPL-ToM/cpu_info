/**
 *	cpu_info - simple example using only cpu_topo component
 *
 */
#include <iostream>
#include <format>
#include <cmath>
#include <chrono>
#include <cpu_topo.hpp>

using namespace cpu_info;
using namespace std;

#define X( name, val ) #name,
constexpr const char *effistr[] = { EFFICIENCY_TYPE( X ) };
constexpr const char *corestr[] = { CORE_TYPE( X ) };
#undef X

int main( int argc, char *argv[] )
{
	return 0;
}
