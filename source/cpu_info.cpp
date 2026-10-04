/**
 * 	cpu_topo:	trying to get
 *
 * 		[Intel's official code](https://github.com/intel/SDM-Processor-Topology-Enumeration)
 *
 * 				ported to a simple cpp class …
 *
 * Step #1✓:	simple cpu enumeration on a system with subclassing by CPU domains
 * 				- query count of CPUs of a domain
 * 				- retrieve (un)masked APIC IDs per cpu
 * 	→ 	Solves the question of "how many threads do make sense in certain scenarios"
 *		by comparing different level counts.
 *	→	[x] feature-complete!
 *
 * 	Step #2✓:	query (if available) core_types (efficiency/performance etc.)
 *
 * 	Step #3✗:	also query "memory"-items, so scoring by shared / non-shared ids becomes possible.
 *
 */

#ifdef _WIN32
	#define NOMINMAX
	#include <Windows.h>
#elifdef linux
	#include <sys/sysinfo.h>
#endif

#include <cpu_info.h>
#include <ranges>

namespace cpu_info
{
	cpu_topo::cpu_topo()
		: process_affinity()
	{
		if ( !cpu_set( 0 ).applyToCurrentThread() ) // check if switching works
			throw "cannot switch cpu affinity, no fallback available.";
		refresh();
	}

	void cpu_topo::refresh() noexcept
	{
		cpu_ids.clear();
		const auto cnt = cpu_set::get_logical_cpu_count();
		for ( auto n: views::iota( 0u, cnt ) )
			cpu_set( n ).applyToCurrentThread(), cpu_ids.emplace_back();
		// reset CPU affinity to before
		process_affinity.applyToCurrentThread();

		parse_topology();
	}

	int cpu_topo::level_shift( cpu_domain d ) const noexcept
	{
		const auto &cpu0 = cpu_ids.front();
		const auto	src	 = id_leaf();
		if ( src == 1 )
		{
			if ( !cpu0( cpu_feature::HTT ) ) return create_topology_shift( 1 );
			else
			{
				const auto MaximumAddressibleIdsPhysicalPackage =
					( unsigned ) ( ( cpu0[ 1 ][ 0 ].e.bx >> 16 ) & 0xFF );

				// This would be a 20+ year old platform to not support CPUID.4 … You cannot report
				// Cores here, a Package == Core and so this only reports SMT within a Package.
				if ( cpu0.max_leaf() < 4 )
					return create_topology_shift( MaximumAddressibleIdsPhysicalPackage );
				else
				{ /* MaximumAddressibleIdsCores: CPUID.4.0.EAX[31:26] */
					const auto MaximumAddressibleIdsCores = ( cpu0[ 4 ][ 0 ].e.ax >> 26 ) + 1;
					// Determine the number of LogicalProcessors per core.
					if ( d == cpu_domain::LogicalDomain )
						return create_topology_shift( MaximumAddressibleIdsPhysicalPackage
													  / MaximumAddressibleIdsCores );
					else // the top level shift is always the same
						return create_topology_shift( MaximumAddressibleIdsPhysicalPackage );
				}
			}
		} else
		{
			const auto &sl = cpu0[ id_leaf() ];
			unsigned	sub{ 0 };
			for ( ; sl[ sub ].e.bx != 0; ++sub )
			{
				// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
				// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
				if ( d == ( ( sl[ sub ].e.cx >> 8 ) & 0xFF ) ) return sl[ sub ].e.ax & 0x1F;
			}
			return sl[ sub ].e.ax & 0x1F; // Fallback: top level shift propagates further …
		}
	}

	id_mask cpu_topo::level_mask( cpu_domain d ) const noexcept
	{
		if ( d <= cpu_domain::LogicalDomain ) return -1u;
		else return ~( ( 1 << level_shift( cpu_domain( d - 1 ) ) ) - 1 );
	}

	unsigned cpu_topo::create_topology_shift( unsigned int count ) const noexcept
	{
		unsigned int Shift{ 31u };
		unsigned int Index{ ( 1u << Shift ) };

		count = ( count * 2 ) - 1;
		for ( ; Index; Index >>= 1, Shift-- )
			if ( count & Index ) break;

		return Shift;
	};

	unsigned cpu_topo::id_leaf( int index ) const
	{
		const auto maxl = cpu_ids[ index ].max_leaf();
		return ( maxl >= 0x1f ? 0x1fu : ( maxl >= 0xB ) ? 0xbu : 1 );
	}

	void cpu_topo::parse_topology()
	{
		apicid_bit_layouts abl;
		auto			  &cpu0 = cpu_ids.front();
		if ( id_leaf() == 1 )
		{
			abl.emplace_back( LogicalDomain, level_shift( LogicalDomain ), mask_map{} );
			abl.emplace_back( CoreDomain, level_shift( CoreDomain ), mask_map{} );
			abl.top_domain = ModuleDomain;
		} else
		{
			const auto &sl = cpu0[ id_leaf() ];
			for ( unsigned sub{ 0 }; sl[ sub ].e.bx != 0; ++sub )
			{
				// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
				const auto DomainType  = ( sl[ sub ].e.cx >> 8 ) & 0xFF;
				// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
				const auto DomainShift = sl[ sub ].e.ax & 0x1F;
				/*
				 * Best to check for known domains explicity since
				 * the ones you use may not be in sequential ordering.
				 */
				switch ( DomainType )
				{
					case InvalidDomain:
						/*  This would be an error, could log it. */
					case LogicalDomain:
					case CoreDomain:
					case ModuleDomain:
					case TileDomain:
					case DieDomain:
					case DieGrpDomain:
						abl.emplace_back( ( cpu_domain ) DomainType, DomainShift, mask_map{} );
						break;
						// First Domain is always Logical Processor,
						// so we will always have a valid previous.
					default:
						abl.back().shift = DomainShift;
						abl.top_domain	 = ( cpu_domain ) DomainType;
				}
			}
		}
		// second step: produce relative apic_id_masks from retrieved information
		unsigned	index{}, nxt_index{}, prev_bit{}, top_domain{ ( unsigned ) abl.size() };
		unsigned	domain_shift, cpu_cnt{ ( unsigned ) cpu_ids.size() };
		const auto &ca{ abl };
		for ( ; index < top_domain; ++index )
		{	// previous shift makes up current mask (see level_mask() implementation)
			abl[ index ].relative_masks.emplace( index, ~( ( 1 << prev_bit ) - 1 ) );
			prev_bit = ca[ index ].shift;
		}
		for ( index = 0u; index < top_domain; ++index )
			for ( nxt_index = index + 1; nxt_index <= top_domain; ++nxt_index )
				abl[ index ].relative_masks.emplace(
					nxt_index,
					( ~ca[ nxt_index ].relative_masks[ nxt_index ] )
						& ( ca[ index ].relative_masks[ index ] ) );

		// at last, build the counter-map and update all cpu_ids
		lvl_ids.clear(), lvl_ids.resize( top_domain + 1 );
		for ( unsigned cpu{}; cpu < cpu_cnt; cpu++ )
		{
			// id_list id;
			for ( index = 0, domain_shift = 0; index < top_domain; index++ )
			{
				if ( ca[ index ].shift != 0 )
				{
					const auto domain_index =
						( ca[ index ].relative_masks[ top_domain ] & ( apic_id ) cpu_ids[ cpu ] )
						>> domain_shift;
					lvl_ids[ index ][ domain_index ]++;
					// id.push_back( domain_index );
				}
				domain_shift = abl[ index ].shift;
			}
			lvl_ids[ index ]
				   [ ( ca[ top_domain ].relative_masks[ top_domain ] & ( apic_id ) cpu_ids[ cpu ] )
					 >> ca[ top_domain - 1 ].shift ]++;
			// cpu_ids[ cpu ].masked_ids = id;
		}
	}
} // namespace cpu_info
