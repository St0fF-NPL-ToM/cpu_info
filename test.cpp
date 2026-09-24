
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
	cout << "logical:  " << logical << ",\n"
		 << "physical: " << physical << ",\n"
		 << "modules:  " << brot.countLevel( cpu_domain::ModuleDomain ) << endl;
	int tpl{ 8 };
	for ( auto i: views::iota( 0ull, brot.cpu_ids.size() ) )
		cout << format( "{:02d} = ", i ) << ( string ) brot.id( i )
			 << ( --tpl ? ", " : ( tpl = 8, "\n" ) );

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