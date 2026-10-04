
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

// produce constant tables for output
#define X( n ) #n,
constexpr const char* domains[] = { CPU_DOMAINS( X ) };
constexpr const char* types[]	= { PROCESSOR_TYPE( X ) };
#undef X
#define X( n, v ) #n,
constexpr const char* effs[] = { EFFICIENCY_TYPE( X ) };
#undef X
#define X( n, t ) { t, #n },
const map< int, string > cores{ { CORE_TYPE( X ){ 0, "NONE" } } };
#undef X

int main( int argc, const char* argv[] )
{
	cpu_topo   brot;
	const auto md = brot.max_domain();
	for ( auto d{ cpu_domain::LogicalDomain }; d <= md; d = cpu_domain( d + 1 ) )
		cout << format( "{:13s}: {:d}\n", domains[ d ], brot.count_domain( d ) );

	cout << endl << "apic-ids and types:" << endl;
	for ( auto i: views::iota( 0ull, brot.count() ) )
	{
		const auto&	   bi = brot[ i ];
		const auto	   id = ( apic_id ) bi;
		const unsigned mv =
			( bi.family() << 16 ) | ( bi.model() << 8 ) | ( bi.type() << 4 ) | bi.stepping();
		vector< string > mi;
		for ( cpu_domain ii{ cpu_info::LogicalDomain }; ii < md; ii = cpu_domain( ii + 1 ) )
		{
			const auto s = brot.level_shift( ii );
			const auto m = brot.level_mask( ii );
			mi.push_back(
				format( "{:#06X}", ( id & m ) >> s ) );
		}
		cout << format( "{:02d}: {:#06X} ({:06x}.{:06x}, '{:s}', {:s} ({:s}) ) masked ids: ", i, id,
						mv, bi.core_model(), bi.brand_string(), cores.at( bi.core_type() ),
						effs[ bi.efficiency() ] )
			 << mi << endl;
	}
	return 0;
}
