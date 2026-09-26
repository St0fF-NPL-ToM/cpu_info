
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
	for ( auto i: views::iota( 0ull, brot.cpu_ids.size() ) )
	{
		const auto& bi = brot.id( i );
		cout << format( "{:02d}: {:s}\n", i, ( string ) bi );
	}

	cout << endl << "optimal affinities: " << endl;
	for ( int n( 2 ); n <= physical; n += n )
	{
		cout << "count: " << format( "{:02d}", n ) << " mask:";
		auto ids = brot.optimalProcessAffinity( n, true );
		for ( auto id: ids ) cout << ( string ) brot.id( id ) << ", ";
		cout << endl;
	}
	return 0;
}