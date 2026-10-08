/**
 *	cpu_info - simple example using only cpu_id component
 *
 */
#include <iostream>
#include <format>
#include <cmath>
#include <chrono>
#include <cpu_id.hpp>

using namespace cpu_info;
using namespace std;

#define X( name, val ) #name,
constexpr const char *effistr[] = { EFFICIENCY_TYPE( X ) };
constexpr const char *corestr[] = { CORE_TYPE( X ) };
#undef X

void show( int cpu )
{
	cpu_id c( cpu );
	cout << format( "CPU #{:03d}: '{:s}' APIC_id: {:#06x}\n", cpu, c.brand_string(),
					c.domain_id() );
	cout << format( "- Family: {:#04x}, Model: {:#04x}, Stepping: {:#04x}\n", c.family(), c.model(),
					c.stepping() );
	cout << format( "- internal type: {:s}, model {:#x} -> efficiency: {:s}\n",
					corestr[ c.core_type() >> 4 ], c.core_model(),
					effistr[ ( int ) c.efficiency() ] );
}

int main( int argc, char *argv[] )
{
	const auto logical{ count() };
	const auto random{ std::chrono::system_clock::now().time_since_epoch().count() };
	std::cout << std::format(
		"'{:s}' => Logical cpu cores: {:d}, showing first, random, last ...\n",
		brand_string< CPUID >(), logical );
	show( 0 );
	show( std::min( logical - 1, // this is demoscene-random-noise: take the time, use a sin.
					int( double( logical >> 1 ) * ( std::sin( double( random ) ) + 1. ) ) ) );
	show( logical - 1 );
	return 0;
}
