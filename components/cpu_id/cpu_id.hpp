#pragma once
/**
 *
 */
#include <cpu_info.hpp>

#include <map>
#include <ranges>
#include <stdexcept>

namespace cpu_info
{

	class cpu_id : public map< unsigned, vector< cpuid_result > >
	{
	  public:
		using M = map< unsigned, vector< cpuid_result > >;
		using L = vector< cpuid_result >;
		using R = cpuid_result;

		using M::map;
		explicit cpu_id( int cpu_number )
			: M()
		{ // get current thread affinity, bind to cpu_number
			auto aff = affinity().read_current();
			aff.inherit( cpu_number ).set_to_current();
			// Read leaf 0, then emplace the rest
			const auto ml = emplace( 0u, L{ cpuid( 0, 0 ) } ).first->second.front().e.ax;
			// create the CPUID-LEAFS map:
			for ( unsigned leaf: views::iota( 0u, ml ) ) retrieve( leaf + 1 );
			// restore affinity
			aff.set_to_current();
		}
		const R &at( unsigned leaf, unsigned subleaf ) const { return M::at( leaf )[ subleaf ]; }
		L		&operator[]( unsigned leaf )
		{
			if ( contains( leaf ) ) return M::operator[]( leaf );
			throw std::out_of_range( "leaf not contained on core" );
		}
		unsigned max_leaf() const noexcept { return ( *this ? at( 0, 0 ).e.ax : 0u ); }
		unsigned id_leaf() const noexcept
		{
			const auto ml = max_leaf();
			return ml ? ml < 0x1f ? ml < 0x0b ? 1 : 0x0b : 0x1f : 0;
		}
		operator bool() const noexcept { return !empty(); }
		operator APIC_id() const noexcept
		{
			const auto sl = id_leaf();
			if ( sl > 1 ) return at( sl, 0 ).e.dx;
			else if ( sl ) return at( 1, 0 ).e.bx >> 24;
			else return -1u; // illegal!
		}
		int domain_shift( cpu_domain domain ) const noexcept
		{
			unsigned il = id_leaf();
			if ( il <= 1 ) // catch "illegal object" as "don't know anything"
			{
				if ( !has_feature( cpu_feature::HTT ) ) return create_topology_shift( 1 );
				else // max_leaf minimum = 1
				{
					const auto MaxIdsPhysical = ( unsigned ) ( ( at( 1, 0 ).e.bx >> 16 ) & 0xFF );
					// This would be a 20+ year old platform to not support CPUID.4 … You cannot
					// report Cores here, a Package == Core and so this only reports SMT within a
					// Package.
					if ( max_leaf() < 4 || domain != cpu_domain::LogicalDomain )
						return create_topology_shift( MaxIdsPhysical );
					else /* MaximumAddressibleIdsCores: CPUID.4.0.EAX[31:26] */
						return create_topology_shift( MaxIdsPhysical
													  / ( ( at( 4, 0 ).e.ax >> 26 ) + 1 ) );
				}
			} else
			{
				unsigned sub{ 0 };
				auto	 sl = at( il, sub );
				for ( ; sl.e.bx != 0; ++sub )
				{
					if ( sub ) sl = at( il, sub );
					if ( domain == ( ( sl.e.cx >> 8 ) & 0xFF ) ) return sl.e.ax & 0x1F;
				}
				return sl.e.ax & 0x1F; // Fallback: top level shift propagates further …
			}
		}
		id_mask domain_mask( cpu_domain domain ) const noexcept
		{
			if ( domain <= cpu_domain::LogicalDomain ) return -1u;
			const auto ps = domain_shift( cpu_domain( domain - 1 ) );
			const auto ds = domain_shift( domain );
			return ( ( 1 << ds ) - 1 ) ^ ( ( 1 << ps ) - 1 );
		}
		APIC_id domain_id( cpu_domain domain ) const noexcept
		{
			return ( ( ( APIC_id ) ( *this ) ) & domain_mask( domain ) )
				   >> domain_shift( cpu_domain( domain - 1 ) );
		}

		bool			   operator()( cpu_feature feature ) const noexcept;
		uint8_t			   stepping() const noexcept;
		uint8_t			   family() const noexcept;
		uint8_t			   model() const noexcept;
		cpu_processor_type type() const noexcept;
		cpu_core_type	   core_type() const noexcept;
		unsigned		   core_model() const noexcept;
		string			   brand_string() const noexcept;
		cpu_efficiency	   efficiency() const noexcept;

	  protected:
		// this is the 2nd part of heavy lifting in "cpuid interpretation"
		// (1st part were the Intel enumerations …)
		void retrieve( unsigned leaf ) noexcept
		{ // clang-format off
            static const set< unsigned >		   reserved{ 0x08, 0x0c, 0x0e, 0x11, 0x13,
            /* unsupported/reserved leaf ids: */			 0x21, 0x22, 0x25, 0x26 };
            static const map< unsigned, unsigned > requirements{
                { 0x09, 0x01000200u + 18 }, // This leaf is valid if CPUID.01H:ECX.DCA[18] = 1
                { 0x0d, 0x01000200u + 26 }, // This leaf is valid if CPUID.01H:ECX.XSAVE[26] = 1
                { 0x0f, 0x07000100u + 12 }, // This leaf is valid if CPUID.07H.00H:EBX.RDT_M[12] = 1
                { 0x10, 0x07000100u + 15 }, // This leaf is valid if CPUID.07H.00H:EBX.RDT_A[15] = 1
                { 0x12, 0x07000100u + 2 },	// This leaf is valid if CPUID.07H.00H:EBX.SGX[2]
                { 0x14, 0x07000100u + 25 }, // This leaf is valid if CPUID.07H.00H:EBX.INTEL_PROC_TRACE[25] = 1
                { 0x19, 0x07000200u + 23 }, // This leaf is valid if CPUID.07H.00H:ECX.KEY_LOCKER[23] = 1
                { 0x1b, 0x07000300u + 18 }, // This leaf is valid if CPUID.07H.00H:EDX.PCONFIG[18] = 1
                { 0x1c, 0x07000300u + 19 }, // This leaf is valid if CPUID.07H.00H:EDX.ARCH_LBRS[19] = 1
                { 0x1d, 0x07000300u + 24 },	// This leaf is valid if CPUID.07H.00H:EDX.AMX_TILE[24] = 1
                { 0x1e, 0x07000300u + 24 },	// This leaf is valid if CPUID.07H.00H:EDX.AMX_TILE[24] = 1
                { 0x20, 0x07010000u + 22 }, // This leaf is valid if CPUID.07H.01H:EAX.HRESET[22] = 1
                { 0x23, 0x07010000u + 8 },	// This leaf is valid if CPUID.07H.01H:EAX.ARCH_PERFMON_EXT[8] = 1
                { 0x24, 0x07010300u + 19 },	// This leaf is valid if CPUID.07H.01H:EDX.AVX10[19] = 1
                { 0x27, 0x07010200u + 0	},	// This leaf is valid if CPUID.07H.01H:ECX.RDT_M_ASYM[0] = 1
                { 0x28, 0x07010200u + 1 },	// This leaf is valid if CPUID.07H.01H:ECX.RDT_A_SYM[1] = 1
            }; // clang-format on
			// any prerequisites to take …
			if ( reserved.contains( leaf ) ) return; // I HATE EARLY OUTS
			if ( requirements.contains( leaf ) )	 // need a check
			{
				const auto &req = requirements.at( leaf );
				if ( ( at( req >> 24 ).at( ( req >> 16 ) & 0xff ).r[ ( req >> 8 ) & 3 ]
					   & ( 1 << ( req & 0x1f ) ) )
					 == 0 )
					return; // I HATE EARLY OUTS
			}
			// ok, so the leaf should be available / valid …
			auto l = emplace( leaf, L{ cpuid( leaf, 0 ) } ).first;
			switch ( leaf )
			{
				case 0x04: // leaf #04 reports 0 within eax[4:0] on the last leaf.
					while ( l->second.back().e.ax & 0x1f )
						l->second.emplace_back( cpuid( leaf, l->second.size() ) );
					break;
				case 0x07: // leafs specifying "max_subleaf" within eax of subleaf 0
				case 0x14:
				case 0x17:
				case 0x18:
				case 0x1d:
				case 0x20:
				case 0x24:
					if ( const auto &n = l->second.front().e.ax; n > 1u )
						for ( unsigned s: views::iota( 1u, n ) )
							l->second.emplace_back( cpuid( leaf, s ) );
					break;
				case 0x0a: // This leaf is valid if CPUID.0AH:EAX[7:0] (Version ID) > 0
					if ( l->second.front().e.ax & 0xff ) break;
					erase( l ); // make bad in case of invalid result
					break;
				case 0x0b: // leaf #0b reports 0 within ebx[15:0] on the last leaf.
				case 0x1f: // leaf #1f reports 0 within ebx[15:0] on the last leaf.
					while ( l->second.back().e.bx & 0xffff )
						l->second.emplace_back( cpuid( leaf, l->second.size() ) );
					break;
				case 0x0d: // leaf #0d is special … subleafs 0 and 1 are always valid.
					l->second.emplace_back( cpuid( leaf, 1 ) );
					break;
				case 0x10: // Sub-leaf n (n ≥ 1) is only valid when (CPUID.10H.00H:EBX[n] == 1)
				case 0x23: // The sub-leaves of this leaf are enumerated by a bitmask specified in
						   // CPUID.23H.00H.EAX[31:0]
				case 0x27: // Sub-leaf n (n ≥ 1) is only valid when (CPUID.27H.00H:EDX[n] == 1).
				case 0x28: // Sub-leaf n (n ≥ 1) is only valid when (CPUID.28H.00H:EBX[n] == 1).
					for ( auto subleaf: views::iota( 1u, 31u ) )
					{
						const auto shifted = ( leaf == 0x27	  ? l->second.front().e.dx
											   : leaf == 0x23 ? l->second.front().e.ax
															  : l->second.front().e.bx )
											 >> subleaf;
						if ( shifted & 1 ) // valid subleaf?
							l->second.emplace_back( cpuid( leaf, subleaf ) );
						else if ( shifted ) // invalid, but valid leafs left?
							l->second.emplace_back();
						if ( ( shifted >> 1 ) == 0 ) break; // no more valid leafs
					}
					break;
				case 0x12: // subleafs 0 and 1 are always valid,
						   // Sub-leaf n (n ≥ 2) is only valid when CPUID.12H.n:EAX[3:0] != 0
					l->second.emplace_back( cpuid( leaf, 1 ) );
					do l->second.emplace_back( cpuid( leaf, l->second.size() ) );
					while ( l->second.back().e.ax & 0xf );
					l->second.pop_back();
					break;
				case 0x1b: // leaf #1b: Sub-leaf n is only valid when CPUID.1BH.n:EAX[11:0] != 0
					while ( l->second.back().e.ax & 0xFFF )
						l->second.emplace_back( cpuid( leaf, l->second.size() ) );
					l->second.pop_back();
					break;
			}
		}
	};

} // namespace cpu_info
