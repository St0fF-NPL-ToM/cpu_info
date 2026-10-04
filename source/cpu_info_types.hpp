#pragma once
/**
 * 	cpu_info_types:
 *          	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of additional helper types, for example replacing some
 *  of INTEL's C-structs with c++ objects.
 * --------------------------------------------------------------------------
 *	→ a map with a const operator[], returning default on non-existing itens
 *	→ a structure to hold all data of one CPU level domain (using that map)
 *	→ a vector with the same option as that map: return an "empty default"
 */
#include <cpu_enums_intel.h>
#include <map>
#include <vector>

namespace cpu_info
{
    /* first up: POD type redeclarations, std::type redeclarations */
	using namespace std;

	using apic_id = unsigned;
	using id_list = vector< apic_id >;
	using id_mask = unsigned;

	constexpr unsigned create_topology_shift( unsigned int count )
	{
		unsigned int Shift{ 31u };
		unsigned int Index{ ( 1u << Shift ) };
		count = ( count * 2 ) - 1;
		for ( ; Index; Index >>= 1, Shift-- )
			if ( count & Index ) break;
		return Shift;
	};

	class mask_map : public map< unsigned, id_mask >
	{
	  public:
		using BASE = map< unsigned, id_mask >;
		using BASE::map;
		id_mask operator[]( unsigned key ) const
		{
			if ( contains( key ) ) return at( key );
			else return 0u;
		}
	};
	struct apicid_bit_layout
	{
		cpu_domain domain{ InvalidDomain };
		unsigned   shift{};
		mask_map   relative_masks;
	};
	class apicid_bit_layouts : public vector< apicid_bit_layout >
	{
		static constexpr unsigned number_of_apic_bits = 32;

	  public:
		cpu_domain		   top_domain{ InvalidDomain };
		unsigned		   domains() const { return size() + 1; }

		// non-const access operator shall emplace/resize on demand!
		apicid_bit_layout &operator[]( size_t index )
		{
			while ( index >= size() )
				emplace_back( ( cpu_domain ) ( back().domain + 1 ), 0u, mask_map{} );
			return vector< apicid_bit_layout >::operator[]( index );
		}
		// likewise
		const apicid_bit_layout operator[]( size_t index ) const
		{
			if ( index < size() ) return vector< apicid_bit_layout >::operator[]( index );
			else return { InvalidDomain, 0, mask_map{} };
		}
	};

} // namespace cpu_info
