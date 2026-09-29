
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

	cout << endl << "apic-ids and types:" << endl;
#define X( n, t ) { t, #n },
	map< int, string > cores{ { CORE_TYPE( X ){ 0, "NONE" } } };
#undef X
#define X( n ) #n,
	vector< string > types{ { PROCESSOR_TYPE( X ) } };
#undef X
#define X( n, v ) #n,
	vector< string > effs{ { EFFICIENCY_TYPE( X ) } };
#undef X
	for ( auto i: views::iota( 0ull, brot.cpu_ids.size() ) )
	{
		const auto&	   bi = brot.id( i );
		const unsigned mv =
			( bi.family() << 16 ) | ( bi.model() << 8 ) | ( bi.type() << 4 ) | bi.stepping();
		cout << format( "{:02d}: {:s} ({:06x}.{:06x}, '{:s}', {:s} ({:s}) )\n", i, ( string ) bi,
						mv, bi.coreModel(), bi.brand_string(), cores[ bi.coreType() ],
						effs[ bi.efficiency() ] );
	}
	return 0;
}
