/**
 * 	cpu_topo:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
 *
 * Step #1✓:		simple cpu enumeration on a system with subclassing by CPU domains
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
#	define NOMINMAX
#	include <Windows.h>
#endif

#include "cpu_info.h"
#include <ranges>
#include <algorithm>

#ifdef linux
#	include <sched.h>
#	include <unistd.h>
#	include <sys/sysinfo.h>
#	include <math.h>
#endif

namespace cpu_info
{
#pragma region external interface functions

	unsigned get_logical_cpu_count( void ) noexcept
	{
		unsigned NumberOfProcessors{ 1u };
#ifdef _WIN32
		NumberOfProcessors = GetActiveProcessorCount( ALL_PROCESSOR_GROUPS );
#elif defined linux
		NumberOfProcessors = ( unsigned ) get_nprocs();
#endif
		return NumberOfProcessors;
	}

	id_list setThreadAffinity( const id_list &target_ids )
	{
		id_list result;
#ifdef _WIN32
		// Assume the active groups are going to be contiguous.
		GROUP_AFFINITY ga{}, prev{};
		auto		   ct = GetCurrentThread();
		auto		   MaxGroups{ GetActiveProcessorGroupCount() }, selGi{ UINT16_MAX };
		auto		   ProcessorNumber{ 0u };
		// build an affinity mask for the given set of cpus
		for ( uint16_t GroupIndex{ 0u }; GroupIndex < MaxGroups; GroupIndex++ )
		{
			uint32_t NumberOfGroupProcessors = GetActiveProcessorCount( GroupIndex );
			for ( auto GroupProcessorNumber{ 0u }; GroupProcessorNumber < NumberOfGroupProcessors;
				  ++GroupProcessorNumber, ++ProcessorNumber )
			{
				if ( find( target_ids.begin(), target_ids.end(), ProcessorNumber )
					 != target_ids.end() )
				{
					if ( selGi == UINT16_MAX ) selGi = ga.Group = GroupIndex;
					else if ( selGi != GroupIndex )
						break; // next group started - cannot use further ids
					ga.Mask |= ( KAFFINITY ) ( ( ULONG64 ) 1 << ( ULONG64 ) GroupProcessorNumber );
				}
			}
		}
		// set affinity, retrieving previous mask
		if ( SetThreadGroupAffinity( ct, &ga, &prev ) )
		{
			// calculate cpu numbers used in previous affinity mask
			ProcessorNumber = 0u;
			for ( uint16_t GroupIndex{ 0u }; GroupIndex < MaxGroups; GroupIndex++ )
				if ( auto NumberOfGroupProcessors = GetActiveProcessorCount( GroupIndex );
					 prev.Group != GroupIndex )
					ProcessorNumber += NumberOfGroupProcessors;
				else
					for ( int i{ 0 }; i < sizeof( prev.Mask ) * 8; ++i )
						if ( prev.Mask & ( 1 << i ) ) result.push_back( ProcessorNumber + i );
		}

#elif defined linux
		// Get the size of the maximum number of configured processors.
		auto NumberOfProcessors{ get_nprocs_conf() };
		if ( auto *cpu_set = CPU_ALLOC( NumberOfProcessors ) )
		{
			const auto pid	   = getpid();
			const auto SetSize = CPU_ALLOC_SIZE( NumberOfProcessors );
			CPU_ZERO_S( SetSize, cpu_set );
			sched_getaffinity( pid, SetSize, cpu_set );
			for ( auto i: views::iota( 0, NumberOfProcessors ) )
				if ( CPU_ISSET_S( i, SetSize, cpu_set ) ) result.push_back( i );
			CPU_ZERO_S( SetSize, cpu_set );
			for ( auto &i: target_ids )
				if ( i < NumberOfProcessors ) CPU_SET_S( i, SetSize, cpu_set );
			// non-zero result is failure …
			if ( sched_setaffinity( pid, SetSize, cpu_set ) ) result.clear();
			CPU_FREE( cpu_set );
		}
#endif
		return result;
	}

#pragma endregion
#pragma region topology class

	cpu_topo::cpu_topo()
	{
		// step #1: remember current thread affinity. (and bind to first CPU)
		auto AppAffinity = bind_thread_to_cpu( 0 );
		if ( AppAffinity.empty() )
			throw exception( "cannot switch cpu affinity, no fallback available.", -1 );
		else
		{
			// step #2: collect all cpuid-leafs on all logical cpus
			build_idlist();
			// reset CPU affinity to before
			setThreadAffinity( AppAffinity );
			// step #3: parse topology
			parse_topology();
		}
	}

	void cpu_topo::build_idlist()
	{
		const auto cnt = get_logical_cpu_count();
		for ( auto n: views::iota( 0u, cnt ) )
		{
			bind_thread_to_cpu( n );
			cpu_ids.emplace_back();
		}
	}

	void cpu_topo::parse_topology()
	{
		auto	  &cpu0 = cpu_ids.front();
		// first step: parse available information
		const auto maxp = cpu0.max_leafs();
		sourceLeaf		= ( maxp < 0xB ) ? 1u : ( maxp < 0x1f ) ? 0x0b : 0x1f;
		if ( sourceLeaf == 1 )
		{
			/*  MaximumAddressibleIdsPhysicalPackage:	CPUID.1.EBX[23:16]
			 *	requires:	CPUID.1.EDX[28].HTT == 1
			 */
			unsigned shf0, shf1;
			if ( !cpu0( cpu_feature::HTT ) ) shf0 = shf1 = create_topology_shift( 1 );
			else
			{
				auto MaximumAddressibleIdsPhysicalPackage =
					( unsigned ) ( ( cpu0[ 1 ][ 0 ].e.bx >> 16 ) & 0xFF );
				if ( cpu0.max_leafs() < 4 )
				{ // This would be a 20+ year old platform to not support CPUID.4
				  // You cannot report Cores here, a Package == Core and so this only reports SMT
				  // within a Package.
					shf0 = shf1 = create_topology_shift( MaximumAddressibleIdsPhysicalPackage );
				} else
				{ /* MaximumAddressibleIdsCores: CPUID.4.0.EAX[31:26] */
					const auto MaximumAddressibleIdsCores =
						( cpu0[ 4 ][ 0 ].e.ax >> 26 ) + 1;
					// Determine the number of LogicalProcessors per core.
					const auto LogicalProcessorsPerCore =
						MaximumAddressibleIdsPhysicalPackage / MaximumAddressibleIdsCores;
					const auto LogicalProcessorsPerPackage = MaximumAddressibleIdsPhysicalPackage;
					shf0 = create_topology_shift( LogicalProcessorsPerCore );
					shf1 = create_topology_shift( LogicalProcessorsPerPackage );
				}
			}
			abl.emplace_back( LogicalDomain, shf0, mask_map{} );
			abl.emplace_back( CoreDomain, shf1, mask_map{} );
			abl.top_domain = ModuleDomain;
		} else
		{
			const auto &sl = cpu0[ sourceLeaf ];
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
		{
			abl[ index ].relative_masks.emplace( index, ~( ( 1 << prev_bit ) - 1 ) );
			prev_bit = ca[ index ].shift;
		}
		for ( index = 0u; index < top_domain; ++index )
			for ( nxt_index = index + 1; nxt_index <= top_domain; ++nxt_index )
				abl[ index ].relative_masks.emplace(
					nxt_index,
					( ~ca[ nxt_index ].relative_masks[ nxt_index ] )
						& ( ca[ index ].relative_masks[ index ] ) );
		// produce topology masks depending on what we got
		for ( ; index <= top_domain; index++ )
			if ( ca[ index ].shift != 0 )
				level_masks_names.emplace(
					ca[ index ].domain,
					make_pair( ca[ index ].relative_masks[ index ],
							   index == top_domain ? "_pkg_"
												   : lvl_base_names[ ca[ index ].domain ] ) );
		// at last, build the counter-map and update all cpu_ids
		lvl_ids.resize( top_domain + 1 );
		for ( unsigned cpu{}; cpu < cpu_cnt; cpu++ )
		{
			id_list id;
			for ( index = 0, domain_shift = 0; index < top_domain; index++ )
			{
				if ( ca[ index ].shift != 0 )
				{
					const auto domain_index =
						( ca[ index ].relative_masks[ top_domain ] & ( apic_id ) cpu_ids[ cpu ] )
						>> domain_shift;
					lvl_ids[ index ][ domain_index ]++;
					id.push_back( domain_index );
				}
				domain_shift = abl[ index ].shift;
			}
			lvl_ids[ index ]
				   [ ( ca[ top_domain ].relative_masks[ top_domain ] & ( apic_id ) cpu_ids[ cpu ] )
					 >> ca[ top_domain - 1 ].shift ]++;
			cpu_ids[ cpu ].masked_ids = id;
		}
	}

	unsigned int cpu_topo::create_topology_shift( unsigned int count )
	{
		unsigned int Shift{ 31u };
		unsigned int Index{ ( 1u << Shift ) };

		count = ( count * 2 ) - 1;
		for ( ; Index; Index >>= 1, Shift-- )
			if ( count & Index ) break;

		return Shift;
	}

	int cpu_topo::countLevel( cpu_domain lvl ) const noexcept
	{
		if ( lvl == cpu_domain::InvalidDomain || lvl_ids.size() < ( size_t ) lvl ) return 1;
		return lvl_ids[ lvl - 1 ].size();
	}

	const cpu_id &cpu_topo::id( size_t index ) const
	{
		if ( index < cpu_ids.size() ) return cpu_ids[ index ];
		throw exception( "cpu_id index out of bounds" );
	}

	id_list cpu_topo::optimalProcessAffinity( int thread_count, bool prefer_performance )
	{
		id_list ids;
		// prefer_performance means cores, that do not share logical CPUs
		if ( prefer_performance )
		{
		} else
		{}
		return ids;
	}

#pragma endregion
} // namespace cpu_info