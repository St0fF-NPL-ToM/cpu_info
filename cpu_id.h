/**
 * 	cpu_id:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_id subclass, also implementing the cpuid
 *  instruction operations.
 */
#pragma once

#include <cpu_enums.h>
#include <vector>
#include <map>
#include <string>
#include <cstdint>

namespace cpu_info
{
	using namespace std;

	using apic_id = unsigned;
	using id_list = vector< apic_id >;
	using id_mask = unsigned;
	/**
	 * 	Result of a cpuid - instruction (simply 4 32bit registers)
	 */
	union cpuid_result
	{
		unsigned int r[ 4 ];
		struct
		{
			unsigned int ax, bx, cx, dx;
		} e;
	};
	extern cpuid_result call_cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;

	/**
	 *	The CPUID leafs are all queried upon creation!  Thus, the CURRENT LOGICAL CPU
	 *	is queried.  Please make sure, thread affinity is bound to this logical cpu.
	 *
	 *	Why an extra class?
	 *	→	Maps create a new entry upon using the []-operator - BUT WE MUST NOT do that.
	 *		So the operator needs an overload to neither throw, nor create an invalid entry.
	 *		Our overload simply returns an invalid entry.
	 */
	class cpuid_leafs : public map< unsigned, vector< cpuid_result > >
	{
	  public:
		using M = map< unsigned, vector< cpuid_result > >;
		using L = vector< cpuid_result >;
		cpuid_leafs();
		L &operator[]( unsigned leaf ) noexcept
		{
			if ( contains( leaf ) ) return M::operator[]( leaf );
			else return _invalid;
		}
		unsigned max_leafs() const noexcept { return _maxLeaf; }

	  protected:
		L			  &retrieve( unsigned leaf ) noexcept;
		static L	   _invalid;
		const unsigned _maxLeaf{ 0 }; // initialized upon construction!
	};

	/**
	 * 	This class is made to describe one logical CPU in your system
	 * 	→ its APIC_ID
	 * 	→ a list of this id masked and shifted as different domain IDs
	 * 	→ the content of CPUID(1) — showing CPU caps (testable)
	 */
	class cpu_id : public cpuid_leafs
	{
		friend class cpu_topo;
		static int fmt_width;	 // static - will be set according to maximum apic_id encountered.
		id_list	   masked_ids{}; // domain-masked apic_ids
	  public:
						   operator bool() const noexcept { return !empty(); }
						   operator apic_id() const noexcept;
						   operator string() const noexcept;
		bool			   operator()( cpu_feature feature ) const;
		uint8_t			   stepping() const noexcept;
		uint8_t			   family() const noexcept;
		uint8_t			   model() const noexcept;
		cpu_processor_type type() const noexcept;
		cpu_core_type	   coreType() const noexcept;
		unsigned		   coreModel() const noexcept;
	};

	/*	replacing INTEL's C-structs with some OOP
	 * -------------------------------------------
	 *	→ a map with a const operator[], returning default on non-existing itens
	 *	→ a structure to hold all data of one CPU level domain (using that map)
	 *	→ a vector with the same option as that map: return an "empty default"
	 */
	class mask_map : public map< unsigned, id_mask >
	{
	  public:
		using BASE = map< unsigned, id_mask >;
		using BASE::map;
		const id_mask operator[]( unsigned key ) const
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