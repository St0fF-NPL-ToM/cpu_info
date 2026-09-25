
#include <iostream>
#include "cpu_info.h"
#include <ranges>

using namespace std;
using namespace cpu_info;

template < typename T >
ostream& operator<<( ostream& s, const vector< T >& v )
{
	auto it = v.begin();
	while ( it != v.end() ) s << *it << ( ++it == v.end() ? "" : ", " );
	return s;
}

int main( int argc, const char* argv[] )
{
	cpu_topo   brot;
	const auto logical	= brot.countLevel( cpu_domain::LogicalDomain );
	const auto physical = brot.countLevel( cpu_domain::CoreDomain );
	cout << "logical : " << logical << ",\n"
		 << "physical: " << physical << ",\n"
		 << "modules : " << brot.countLevel( cpu_domain::ModuleDomain ) << endl;

	cout << endl << "apic-ids and feature maps:" << endl;
	const std::vector< cpu_feature > features{
		cpu_feature::ACPI,	cpu_feature::MMX,	 cpu_feature::SSE,	 cpu_feature::SSE2,
		cpu_feature::AVX2,	cpu_feature::SSE3,	 cpu_feature::SSSE3, cpu_feature::FMA,
		cpu_feature::SSE41, cpu_feature::HYBRID, cpu_feature::SSE42, cpu_feature::AESNI,
		cpu_feature::AVX,	cpu_feature::F16C,	 cpu_feature::AVX10, cpu_feature::HTT,
		cpu_feature::TSC,	cpu_feature::TM,	 cpu_feature::TM2,	 cpu_feature::MCDT_NO };
	for ( auto i: views::iota( 0ull, brot.cpu_ids.size() ) )
	{
		bind_thread_to_cpu( i );
		unsigned fm{ 0u };
		for ( auto f: features ) fm <<= 1, fm |= unsigned( brot.cpu_ids[ i ]( f ) );
		cout << format( "{:02d}: {:s}={:032B}, core = {:02x}, model = {:02x}\n", i,
						( string ) brot.id( i ), fm, brot.id( i ).core_type,
						brot.id( i ).model_id );
	}

	cout << endl << "optimal affinities: " << endl;
	for ( int n( 2 ); n <= physical; n += n )
	{
		cout << "count: " << format( "{:02d}", n ) << " mask:";
		auto ids = brot.optimalProcessAffinity( n, true );
		for ( auto id: ids ) cout << brot.cpu_ids.at( id ) << ", ";
		cout << endl;
	}
	return 0;
}