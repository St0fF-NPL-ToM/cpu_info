#pragma once
/**
 *	cpu_info - cpu_id component:
 *	============================
 *
 *	This component enriches cpu_info with the cpu_id class.  It is basically a data-holder for
 *	one single logical cpu core.
 *
 *	only one CTor: (should default-CTor get deleted?)
 */
#include "cpu_info.hpp"

#include <map>
#include <ranges>

namespace cpu_info
{
	class cpu_id : public std::map< unsigned, std::vector< cpuid_result > >
	{
	  public:
		using M = std::map< unsigned, std::vector< cpuid_result > >;
		using L = std::vector< cpuid_result >;
		using R = cpuid_result;

		operator bool() const noexcept { return !M::empty(); }
		// CTor that does simply run all calls - CALLEE HAS TO SET AFFINITIES!
		cpu_id()
			: M()
		{ retrieve_all(); }
		// A specific CPU number is requested: takes care about affinity mask itself!
		cpu_id( int cpu_number )
			: M()
		{
			auto aff = affinity().read_current();
			aff.inherit( cpu_number ).set_to_current();
			retrieve_all();
			// restore affinity
			aff.set_to_current();
		}
		// function to prepare "being a cpuid accessor"
		R at( unsigned leaf, unsigned subleaf ) const noexcept
		{
			if ( contains( leaf ) )
			{
				const auto l = M::at( leaf );
				if ( l.size() > subleaf ) return l[ subleaf ];
			}
			return {};
		}
		// accessor to meet the requirement to use the template functions from cpu_info
		R operator()( unsigned l, unsigned s ) const noexcept { return at( l, s ); }
		// overwrite default map index operator with this const version
		L operator[]( unsigned leaf ) const noexcept
		{
			if ( contains( leaf ) ) return M::at( leaf );
			else return {};
		}
		// Informational functionality following …
		unsigned max_leaf() const noexcept { return ( *this ? at( 0, 0 ).e.ax : 0u ); }
		unsigned id_leaf() const noexcept { return cpu_info::id_leaf( *this ); }

				 operator APIC_id() const noexcept
		{ return cpu_info::apic_id( cpu_domain::LogicalDomain, *this ); }

		int domain_shift( cpu_domain domain ) const noexcept
		{ return cpu_info::domain_shift( domain, *this ); }

		id_mask domain_mask( cpu_domain domain ) const noexcept
		{ return cpu_info::domain_mask( domain, *this ); }

		APIC_id domain_id( cpu_domain domain = cpu_domain::LogicalDomain ) const noexcept
		{ return cpu_info::apic_id( domain, *this ); }

		bool operator()( cpu_feature feature ) const noexcept
		{ return cpu_info::has_feature( feature, *this ); }

		uint8_t			   stepping() const noexcept { return at( 1, 0 ).e.ax & 0b1111; }
		uint8_t			   family() const noexcept { return cpu_info::family( *this ); }
		uint8_t			   model() const noexcept { return cpu_info::model( *this ); }
		cpu_processor_type type() const noexcept { return cpu_info::type( *this ); }
		cpu_core_type	   core_type() const noexcept { return cpu_info::core_type( *this ); }
		unsigned		   core_model() const noexcept { return cpu_info::core_model( *this ); }
		cpu_efficiency	   efficiency() const noexcept { return cpu_info::efficiency( *this ); }
		// brand string is special: while the index-method works with standard cpuid leafs,
		// the brand string method on the other hand uses extended (negative) leafs.
		// Thus: instantiate a CPUID query object instead of reading empty data.
		std::string		   brand_string() const noexcept { return cpu_info::brand_string( *this ); }

	  protected:
		void retrieve_all() noexcept
		{
			CPUID	   get; // a Reader-Accessor
			const auto ml = emplace( 0u, L{ get( 0, 0 ) } ).first->second.front().e.ax;
			// create the CPUID-LEAFS map:
			for ( unsigned leaf: std::views::iota( 0u, ml ) )
				if ( cpu_info::leaf_valid( leaf + 1, *this ) ) retrieve( leaf + 1, get );

			// also, read the extended leafs - they all only provide subleaf #0
			const auto en = // how many are there?
				emplace( ext_index, L{ get( ext_index, 0 ) } ).first->second.front().e.ax;
			for ( unsigned el: std::views::iota( ext_index + 1u, en ) )
				emplace( el, L{ get( el, 0 ) } );
		}
		// After the Intel enumeration and special types, this is the other part of heavy lifting
		// regarding the CPUID instruction:	Task at hand = "acquire one leaf and all its subleafs"
		//	→	make sure, that leaf is present and NOT "reserved"
		//	→	in case there are requirements for a leaf to exist, these need to be checked as well
		// 	→	then, subleaf 0 can be emplaced
		//		- on occasion, this leaf even tells, if it is valid (I will keep it so the test)
		//	→	subleafs? This is also handled very differently …
		//		- some specify an exact amount of existing subleafs
		//		- some have always valid amounts of subleafs
		//		- some subleafs report "I am the last subleaf"
		//
		void retrieve( unsigned leaf, const CPUID &get ) noexcept
		{
			// ok, so the leaf should be available / valid …
			auto l = emplace( leaf, L{ get( leaf, 0 ) } ).first;
			switch ( leaf )
			{
				case 0x04: // leaf #04 reports 0 within eax[4:0] on the last leaf.
					while ( l->second.back().e.ax & 0x1f )
						l->second.emplace_back( get( leaf, l->second.size() ) );
					break;
				case 0x07: // leafs specifying "max_subleaf" within eax of subleaf 0
				case 0x14:
				case 0x17:
				case 0x18:
				case 0x1d:
				case 0x20:
				case 0x24:
					if ( const auto &n = l->second.front().e.ax; n > 1u )
						for ( unsigned s: std::views::iota( 1u, n ) )
							l->second.emplace_back( get( leaf, s ) );
					break;
				case 0x0a: // This leaf is valid if CPUID.0AH:EAX[7:0] (Version ID) > 0
					break;
				case 0x0b: // leaf #0b reports 0 within ebx[15:0] on the last leaf.
				case 0x1f: // leaf #1f reports 0 within ebx[15:0] on the last leaf.
					while ( l->second.back().e.bx & 0xffff )
						l->second.emplace_back( get( leaf, l->second.size() ) );
					break;
				case 0x0d: // leaf #0d is special … subleafs 0 and 1 are always valid.
					l->second.emplace_back( get( leaf, 1 ) );
					break;
				case 0x10: // Sub-leaf n (n ≥ 1) is only valid when (CPUID.10H.00H:EBX[n] == 1)
				case 0x23: // The sub-leaves of this leaf are enumerated by a bitmask specified
						   // in CPUID.23H.00H.EAX[31:0]
				case 0x27: // Sub-leaf n (n ≥ 1) is only valid when (CPUID.27H.00H:EDX[n] == 1).
				case 0x28: // Sub-leaf n (n ≥ 1) is only valid when (CPUID.28H.00H:EBX[n] == 1).
					for ( auto subleaf: std::views::iota( 1u, 31u ) )
					{
						const auto shifted = ( leaf == 0x27	  ? l->second.front().e.dx
											   : leaf == 0x23 ? l->second.front().e.ax
															  : l->second.front().e.bx )
											 >> subleaf;
						if ( shifted & 1 ) // valid subleaf?
							l->second.emplace_back( get( leaf, subleaf ) );
						else if ( shifted ) // invalid, but valid leafs left?
							l->second.emplace_back();
						if ( ( shifted >> 1 ) == 0 ) break; // no more valid leafs
					}
					break;
				case 0x12: // subleafs 0 and 1 are always valid,
						   // Sub-leaf n (n ≥ 2) is only valid when CPUID.12H.n:EAX[3:0] != 0
					l->second.emplace_back( get( leaf, 1 ) );
					do l->second.emplace_back( get( leaf, l->second.size() ) );
					while ( l->second.back().e.ax & 0xf );
					// in this case the last subleaf simply notifies about "done",
					// the size() does the same
					l->second.pop_back();
					break;
				case 0x1b: // leaf #1b: Sub-leaf n is only valid when CPUID.1BH.n:EAX[11:0] != 0
					while ( l->second.back().e.ax & 0xFFF )
						l->second.emplace_back( get( leaf, l->second.size() ) );
					// same here - validity better checked by size()
					l->second.pop_back();
					break;
			}
		}
	};

} // namespace cpu_info
