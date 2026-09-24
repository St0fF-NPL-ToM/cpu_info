/**
 * 	cpu_topo:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
 */

#ifdef _WIN32
#	define NOMINMAX
#	include <Windows.h>
#endif

#include "cpu_info.h"
#include <list>
#include <ranges>

#ifdef linux
#	include <sched.h>
#	include <sys/sysinfo.h>
#endif

namespace cpu_info
{
	/* static */
	int			 cpu_id::fmt_width{ 2 };
	/* static */
	cpuid_result call_cpuid( unsigned int Leaf, unsigned int Subleaf ) noexcept
	{
		cpuid_result CpuidRegisters{};
#if _WIN32
		__cpuidex( reinterpret_cast< int * >( &CpuidRegisters.x.Registers[ 0 ] ), Leaf, Subleaf );

#elif linux
		unsigned int ReturnEax;
		unsigned int ReturnEbx;
		unsigned int ReturnEcx;
		unsigned int ReturnEdx;

		asm( "movl %4, %%eax\n"
			 "movl %5, %%ecx\n"
			 "CPUID\n"
			 "movl %%eax, %0\n"
			 "movl %%ebx, %1\n"
			 "movl %%ecx, %2\n"
			 "movl %%edx, %3\n"
			 : "=r"( ReturnEax ), "=r"( ReturnEbx ), "=r"( ReturnEcx ), "=r"( ReturnEdx )
			 : "r"( Leaf ), "r"( Subleaf )
			 : "%eax", "%ebx", "%ecx", "%edx" );

		CpuidRegisters.x.Register.Eax = ReturnEax;
		CpuidRegisters.x.Register.Ebx = ReturnEbx;
		CpuidRegisters.x.Register.Ecx = ReturnEcx;
		CpuidRegisters.x.Register.Edx = ReturnEdx;
#else
#endif
		return CpuidRegisters;
	}
	/* static */
	void bind_thread_to_cpu( unsigned ProcessorNumber )
	{
#ifdef _WIN32
		GROUP_AFFINITY GroupAffinity{};
		unsigned short GroupIndex{ 0u };
		unsigned short MaxGroups{ GetActiveProcessorGroupCount() };
		unsigned int   NumberOfGroupProcessors;
		// Assume the active groups are going to be contiguous.
		for ( ; GroupIndex < MaxGroups; GroupIndex++ )
			if ( auto NumberOfGroupProcessors = GetActiveProcessorCount( GroupIndex );
				 ProcessorNumber < NumberOfGroupProcessors )
			{
				GroupAffinity.Group = GroupIndex;
				GroupAffinity.Mask = ( KAFFINITY ) ( ( ULONG64 ) 1 << ( ULONG64 ) ProcessorNumber );
				SetThreadGroupAffinity( GetCurrentThread(), &GroupAffinity, NULL );
				break;
			} else ProcessorNumber = ProcessorNumber - NumberOfGroupProcessors;
#elif defined linux
		// Get the size of the maximum number of configured processors.
		auto NumberOfProcessors{ get_nprocs_conf() };
		// Something larger comes in we will just go with it.
		if ( ProcessorNumber > NumberOfProcessors ) NumberOfProcessors = ProcessorNumber;

		if ( auto *cpu_set = CPU_ALLOC( NumberOfProcessors ) )
		{
			const auto SetSize = CPU_ALLOC_SIZE( NumberOfProcessors );
			CPU_ZERO_S( SetSize, cpu_set );
			CPU_SET_S( ProcessorNumber, SetSize, cpu_set );
			sched_setaffinity( getpid(), SetSize, cpu_set );
			CPU_FREE( cpu_set );
		}
#else
#endif
	}
	/* static */
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

	cpu_topo::cpu_topo( bool force_legacy )
	{
		// For initialization, setup globals
		auto CpuidRegisters = call_cpuid( 0, 0 );
		populate_apic_ids( CpuidRegisters );
		if ( CpuidRegisters.x.Register.Eax < 0xB || force_legacy )
			parse_cpuid_legacy( CpuidRegisters );
		else
		{
			sourceLeaf = CpuidRegisters.x.Register.Eax >= 0x1F ? 0x1F : 0x0B;
			parse_cpuid_modern();
		}
		finish_topology();
	}

	int cpu_topo::countLevel( cpu_domain lvl ) const noexcept
	{
		if ( lvl == cpu_domain::InvalidDomain || lvl_ids.size() < ( size_t ) lvl ) return 1;
		return lvl_ids[ lvl - 1 ].size();
	}

	cpu_id cpu_topo::id( size_t index ) const noexcept
	{
		cpu_id result;
		if ( index < cpu_ids.size() ) result = cpu_ids[ index ];
		return result;
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

	void cpu_topo::parse_cpuid_legacy( const cpuid_result &zero_zero )
	{
		unsigned int MaximumAddressibleIdsPhysicalPackage{ 1 };
		unsigned int MaximumAddressibleIdsCores{};
		unsigned int LogicalProcessorsPerCore{ 1 };
		unsigned int LogicalProcessorsPerPackage{ 1 };
		unsigned int PackageShift{};
		unsigned int LogicalProcessorShift{};
		cpuid_result CpuidRegisters{ call_cpuid( 1, 0 ) };
		/*  MaximumAddressibleIdsPhysicalPackage
		 *
		 *      CPUID.1.EBX[23:16]
		 *      Maximum number of addressable IDs for logical processors in this physical package
		 *
		 *  This is the legacy value for determining the package mask and has been superceded by
		 * Leaf 0Bh and Leaf 01Fh. Since this is a byte, processors are already exceeding 256
		 * addressible IDs either due to topology domains or simply having more processors in a
		 * package.
		 *
		 *      CPUID.1.EDX[28].HTT
		 *      The Maximum number of addressable IDs for logical processor in this package is valid
		 * when set to 1.
		 */

		// Determine that CPUID.1.EDX[28].HTT == 1, if this is not set it would be a very old
		// platform.
		if ( CpuidRegisters.x.Register.Edx & ( ( unsigned int ) 1 << 28 ) )
		{
			MaximumAddressibleIdsPhysicalPackage =
				( unsigned int ) ( ( CpuidRegisters.x.Register.Ebx >> 16 ) & 0xFF );
			// This would be a 20+ year old platform to not support CPUID.4
			if ( zero_zero.x.Register.Eax >= 4 )
			{
				/* MaximumAddressibleIdsCores
				 *
				 *      CPUID.4.0.EAX[31:26]
				 *       Maximum number of addressable IDs for processor cores in the physical
				 * Package
				 *
				 *  This is the legacy value for determining the core/SMT mask and has been
				 * superceded by Leaf 0Bh and Leaf 01Fh. Since this is 6 bits, processors are
				 * already exceeding this value addressible IDs either due to topology domains or
				 *  simply having more processors in a package.
				 */
				MaximumAddressibleIdsCores =
					( unsigned int ) ( call_cpuid( 4, 0 ).x.Register.Eax >> 26 ) + 1;
				// Determine the number of LogicalProcessors per core.
				LogicalProcessorsPerCore =
					MaximumAddressibleIdsPhysicalPackage / MaximumAddressibleIdsCores;
				LogicalProcessorsPerPackage = MaximumAddressibleIdsPhysicalPackage;
				LogicalProcessorShift		= create_topology_shift( LogicalProcessorsPerCore );
				PackageShift				= create_topology_shift( LogicalProcessorsPerPackage );
			} else
			{ // You cannot report Cores here, a Package == Core and so this only reports SMT within
			  // a Package.
				LogicalProcessorsPerCore	= MaximumAddressibleIdsPhysicalPackage;
				LogicalProcessorsPerPackage = MaximumAddressibleIdsPhysicalPackage;

				LogicalProcessorShift		= PackageShift =
					create_topology_shift( MaximumAddressibleIdsPhysicalPackage );
			}
		} else // You do not report Cores or SMT here.  It's always 1 Logical Processor.
			LogicalProcessorShift = PackageShift =
				create_topology_shift( MaximumAddressibleIdsPhysicalPackage );

		abl.emplace_back( LogicalDomain, LogicalProcessorShift, mask_map{} );
		abl.emplace_back( CoreDomain, PackageShift, mask_map{} );
		abl.top_domain = ModuleDomain;
	}

	void cpu_topo::parse_cpuid_modern()
	{
		unsigned int Subleaf{ 0 };
		unsigned int DomainType{};
		unsigned int DomainShift{};
		cpuid_result CpuidRegisters{ call_cpuid( sourceLeaf, Subleaf ) };

		while ( CpuidRegisters.x.Register.Ebx != 0 )
		{
			// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
			DomainType	= ( CpuidRegisters.x.Register.Ecx >> 8 ) & 0xFF;
			// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
			DomainShift = CpuidRegisters.x.Register.Eax & 0x1F;
			/*
			 * Best to check for known domains explicity since the ones you use
			 * may not be in sequential ordering.
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

				default:
					// First Domain is always Logical Processor, so we will always have a valid
					// previous.
					abl.back().shift = DomainShift;
					abl.top_domain	 = ( cpu_domain ) DomainType;
			}
			CpuidRegisters = call_cpuid( sourceLeaf, ++Subleaf );
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

	void cpu_topo::populate_apic_ids( cpuid_result &CpuidRegisters )
	{
		// Determine X2APIC ID or fall back to APIC ID.
		auto NumberOfProcessors{ get_logical_cpu_count() };
		auto id{ std::min( CpuidRegisters.x.Register.Eax, 0x1Fu ) };

		cpu_id::fmt_width = int( log2( NumberOfProcessors - 1 ) / 4 ) + 1;
		// is this really necessary?
		if ( NumberOfProcessors > MAX_PROCESSORS ) NumberOfProcessors = MAX_PROCESSORS;

		if ( id < 0x1fu && ( id = std::min( id, 0xbu ) ) < 0x0b ) id = 1;
		apic_cpu_ids.clear();
		for ( auto Index{ 0u }; Index < NumberOfProcessors; Index++ )
		{
			bind_thread_to_cpu( Index );
			unsigned int ApicId{ UINT_MAX };
			cpuid_result CpuidRegistersApicid{ call_cpuid( id, 0 ) };
			if ( id == 0x1F )
			{
				if ( CpuidRegistersApicid.x.Register.Ebx != 0 )
					ApicId = CpuidRegistersApicid.x.Register.Edx;
				else CpuidRegistersApicid = call_cpuid( ( id = 0x0bu ), 0 );
			}
			if ( id == 0x0B )
			{
				if ( CpuidRegistersApicid.x.Register.Ebx != 0 )
					ApicId = CpuidRegistersApicid.x.Register.Edx;
				else CpuidRegistersApicid = call_cpuid( ( id = 1 ), 0 );
			}
			if ( id == 1 ) // Fall back to Legacy 8 bit APIC ID.
				ApicId = ( CpuidRegistersApicid.x.Register.Ebx >> 24 );
			apic_cpu_ids.push_back( ApicId );
		}
	}

	void cpu_topo::finish_topology()
	{
		unsigned	index{}, nxt_index{}, prev_bit{}, top_domain{ ( unsigned ) abl.size() };
		unsigned	domain_shift, cpu_cnt{ ( unsigned ) apic_cpu_ids.size() };
		const auto &ca{ abl };
		for ( ; index < top_domain; ++index )
		{
			abl[ index ].relative_masks.emplace( index, ~( ( 1 << prev_bit ) - 1 ) );
			prev_bit = ca[ index ].shift;
		}
		for ( index = 0u; index < top_domain; ++index )
			for ( nxt_index = index + 1; nxt_index <= top_domain; ++nxt_index )
				abl[ index ].relative_masks.emplace(
					nxt_index, ( ~ca[ nxt_index ].relative_masks[ nxt_index ] )
								   & ( ca[ index ].relative_masks[ index ] ) );

		// produce topology masks depending on what we got
		for ( ; index <= top_domain; index++ )
			if ( ca[ index ].shift != 0 )
				level_masks_names.emplace(
					ca[ index ].domain,
					make_pair( ca[ index ].relative_masks[ index ],
							   index == top_domain ? "_pkg_"
												   : lvl_base_names[ ca[ index ].domain ] ) );

		lvl_ids.resize( top_domain + 1 );
		for ( unsigned cpu{}; cpu < cpu_cnt; cpu++ )
		{
			id_list id;
			for ( index = 0, domain_shift = 0; index < top_domain; index++ )
			{
				if ( ca[ index ].shift != 0 )
				{
					const auto domain_index =
						( ca[ index ].relative_masks[ top_domain ] & apic_cpu_ids[ cpu ] )
						>> domain_shift;
					lvl_ids[ index ][ domain_index ]++;
					id.push_back( domain_index );
				}
				domain_shift = abl[ index ].shift;
			}
			lvl_ids[ index ]
				   [ ( ca[ top_domain ].relative_masks[ top_domain ] & apic_cpu_ids[ cpu ] )
					 >> ca[ top_domain - 1 ].shift ]++;
			cpu_ids.emplace_back( make_pair( apic_cpu_ids[ cpu ], id ) );
		}
	}

	void setThreadAffinity( const id_list &target_ids ) {}
} // namespace cpu_info