/**
 *	cpu_info - simple example using only cpu_topo component
 *
 */
#include <cpu_topo.hpp>
#include <iostream>

using namespace cpu_info;
using namespace std;

// I never understood, why stl doesn't provide at least a partial template instantiation like this …
template < typename T >
ostream& operator<<( ostream& s, const vector< T >& v )
{
	auto it = v.begin();
	while ( it != v.end() ) s << *it << ( ++it == v.end() ? "" : ", " );
	return s;
}

/**
 *	The following shows a way to use the X-macros of cpu_enums_intel.h
 *	to provide some useful output data sources:
 */
#define X( n ) #n,
constexpr const char* domains[] = { CPU_DOMAINS( X ) };
constexpr const char* types[]	= { PROCESSOR_TYPE( X ) };
#undef X
#define X( n, v ) #n,
constexpr const char* effs[] = { EFFICIENCY_TYPE( X ) };
#undef X
/**
 *	The CORE_TYPE macro is a little special - please consult the enumerations.
 *
 *	The final enumeration is non-continuous (same applies to the features enumeration),
 *	thus neither a constexpr, nor a vector do make any sense.  A map or unordered_map needs
 *	to be built.
 */
#define X( n, t ) { t, #n },
const map< int, string > cores{ { CORE_TYPE( X ) } };
#undef X

// as recommended, provide a global instance of cpu_topo
cpu_topo info;

int		 main( int argc, char* argv[] )
{
	// Query largest domain, then output the numbers of respective domain members
	const auto md = info.max_domain();
	for ( auto d{ cpu_domain::LogicalDomain }; d <= md; d = cpu_domain( d + 1 ) )
		cout << format( "{:13s}: {:d}\n", domains[ d ], info.count_domain( d ) );

	// further querying options: id's, type information, efficiencies
	cout << endl << "apic-ids and types:" << endl;
	for ( auto i: views::iota( 0ull, info.count() ) )
	{
		const auto&	   bi = info[ i ];
		const auto	   id = bi.domain_id(); // default is LogicalDomain
		const unsigned mv =
			( bi.family() << 16 ) | ( bi.model() << 8 ) | ( bi.type() << 4 ) | bi.stepping();
		vector< string > mi;
		for ( cpu_domain ii{ cpu_info::LogicalDomain }; ii < md; ii = cpu_domain( ii + 1 ) )
			mi.push_back( format( "{:#06X}", bi.domain_id( ii ) ) );

		cout << format( "{:02d}: {:#06X} ({:06x}.{:06x}, '{:s}', {:s} ({:s}) ) masked ids: ", i, id,
						mv, bi.core_model(), bi.brand_string(), cores.at( bi.core_type() ),
						effs[ ( int ) bi.efficiency() ] )
			 << mi << endl;
	}
	return 0;
}
