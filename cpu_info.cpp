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
#include <format>
#include <list>
#include <ranges>

#ifdef linux
#	include <sched.h>
#	include <sys/sysinfo.h>
#endif

namespace cpu_info
{
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

	/*static*/
	const apic_id_bit_layout apicid_bit_layouts::empty_layout{ InvalidDomain, 0, mask_map{} };

	cpu_topo::cpu_topo( bool force_legacy )
	{
		// For initialization, setup globals
		auto CpuidRegisters = call_cpuid( 0, 0 );
		PopulatePlatformApicIds( CpuidRegisters );
		if ( CpuidRegisters.x.Register.Eax < 0xB || force_legacy )
			parse_cpuid_legacy( CpuidRegisters );
		else
		{
			sourceLeaf = CpuidRegisters.x.Register.Eax >= 0x1F ? 0x1F : 0x0B;
			parse_cpuid_modern();
		}
	}

	int cpu_topo::countLevel( cpu_domain lvl ) const noexcept
	{
		if ( lvl == cpu_domain::InvalidDomain || lvl_ids.size() < ( size_t ) lvl ) return 1;
		return lvl_ids[ lvl - 1 ].size();
	}

	cpu_id cpu_topo::id_of( size_t index ) const noexcept
	{
		cpu_id result;
		if ( index < cpu_ids.size() ) result = cpu_ids[ index ];
		return result;
	}

	string cpu_topo::id_string( size_t index ) const noexcept
	{
		string result;
		if ( auto id = id_of( index ) )
		{
			// get formatting info, prepare string
			auto fmts = getFmt();
			result	  = vformat( fmts.second + "(", make_format_args( id.first ) );
			// append pieces to the string
			auto it	  = id.second.rbegin();
			while ( it != id.second.rend() )
				result.append( vformat( fmts.second, make_format_args( *it ) ) )
					.append( ++it != id.second.rend() ? ":" : ")" );
		}
		return result;
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
				LogicalProcessorShift		= CreateTopologyShift( LogicalProcessorsPerCore );
				PackageShift				= CreateTopologyShift( LogicalProcessorsPerPackage );
			} else
			{ // You cannot report Cores here, a Package == Core and so this only reports SMT within
			  // a Package.
				LogicalProcessorsPerCore	= MaximumAddressibleIdsPhysicalPackage;
				LogicalProcessorsPerPackage = MaximumAddressibleIdsPhysicalPackage;

				LogicalProcessorShift		= PackageShift =
					CreateTopologyShift( MaximumAddressibleIdsPhysicalPackage );
			}
		} else // You do not report Cores or SMT here.  It's always 1 Logical Processor.
			LogicalProcessorShift = PackageShift =
				CreateTopologyShift( MaximumAddressibleIdsPhysicalPackage );


		abl.emplace_back( LogicalDomain, LogicalProcessorShift, mask_map{} );
		abl.emplace_back( CoreDomain, PackageShift, mask_map{} );
		abl.top_domain = ModuleDomain;
		create_domain_masks();
		finish_topology();
	}

	void cpu_topo::parse_cpuid_modern()
	{
		unsigned int		  Subleaf{ 0 };
		unsigned int		  DomainType{};
		unsigned int		  DomainShift{};
		cpuid_result		  CpuidRegisters{ call_cpuid( sourceLeaf, Subleaf ) };
		APICID_BIT_LAYOUT_CTX ApicidBitLayoutCtx{ .NumberOfApicIdBits = 32 };

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
					ApicidBitLayoutCtx.ShiftValues[ ApicidBitLayoutCtx.PackageDomainIndex ] =
						DomainShift;
					ApicidBitLayoutCtx.ShiftValueDomain[ ApicidBitLayoutCtx.PackageDomainIndex ] =
						DomainType;
					ApicidBitLayoutCtx.PackageDomainIndex++;
					abl.emplace_back( ( cpu_domain ) DomainType, DomainShift, mask_map{} );
					break;

				default:
					// First Domain is always Logical Processor, so we will always have a valid
					// previous.
					ApicidBitLayoutCtx.ShiftValues[ ApicidBitLayoutCtx.PackageDomainIndex - 1 ] =
						DomainShift;
					abl.back().shift = DomainShift;
					abl.top_domain	 = ( cpu_domain ) DomainType;
			}
			CpuidRegisters = call_cpuid( sourceLeaf, ++Subleaf );
		}
		create_domain_masks( &ApicidBitLayoutCtx );
		ManyDomainFinalize( &ApicidBitLayoutCtx );
	}

	void cpu_topo::create_domain_masks( APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx )
	{
		unsigned int DomainIndex{ 0 };
		unsigned int PreviousBit{ 0 }, prev_bit{ 0 };
		unsigned int NextDomainIndex;

		// Create globally identifiable masks for each domain.
		for ( ; DomainIndex < pApicidBitLayoutCtx->PackageDomainIndex; DomainIndex++ )
		{
			pApicidBitLayoutCtx->DomainRelativeMasks[ DomainIndex ][ DomainIndex ] =
				~( ( 1 << PreviousBit ) - 1 );
			PreviousBit = pApicidBitLayoutCtx->ShiftValues[ DomainIndex ];
			abl[ DomainIndex ].relative_masks.emplace( DomainIndex, ~( ( 1 << prev_bit ) - 1 ) );
			prev_bit = abl[ DomainIndex ].shift;
		}
		// Create a relative identifier for each Domain to another higher level Domain
		for ( DomainIndex = 0; DomainIndex < pApicidBitLayoutCtx->PackageDomainIndex;
			  DomainIndex++ )
		{
			/* Start to create relative IDs to the next level above the current.
			 *
			 *     A relative ID is taking the global ID mask and removing the previous mask (which
			 * is already done) and then removing the mask of the higher level domain, so for
			 * example:
			 *
			 *     A global Logical processor mask would be 0xFFFFFFFF since all logical processors
			 * are the lowest identifier so the entire APIC ID is needed.
			 *
			 *     A global Core mask could be:  0xFFFFFFFE  meaning the Core ID doesn't include the
			 * lower Logical Processor IDs.  This will identify the 2 Logical processors as a core
			 * globally.
			 *
			 *     A global Package mask could be:  0xFFFFFFF8  Meaning we can identify this package
			 * among other packages and this package has 8 logical processors.
			 *
			 *
			 *     To then create a Mask to create an ID relative to Package, we would do
			 * ~(0xFFFFFFF8) & 0xFFFFFFFE  = 0x00000006  Essentially, you remove the ID mask for the
			 * upper domain from the global mask ID for the core.  To create the full ID though you
			 * also need to use the low bit's shift value.
			 *
			 *     (APIC ID & 0x6)>>1 = CORE_ID for the Package.
			 */
			for ( NextDomainIndex = DomainIndex + 1;
				  NextDomainIndex <= pApicidBitLayoutCtx->PackageDomainIndex;
				  NextDomainIndex++ )
			{
				pApicidBitLayoutCtx->DomainRelativeMasks[ DomainIndex ][ NextDomainIndex ] =
					( ~pApicidBitLayoutCtx
						   ->DomainRelativeMasks[ NextDomainIndex ][ NextDomainIndex ] )
					& ( pApicidBitLayoutCtx->DomainRelativeMasks[ DomainIndex ][ DomainIndex ] );
				abl[ DomainIndex ].relative_masks.emplace(
					NextDomainIndex,
					( ~( NextDomainIndex < abl.size()
							 ? abl[ NextDomainIndex ].relative_masks[ NextDomainIndex ]
							 : 0 ) )
						& ( abl[ DomainIndex ].relative_masks[ DomainIndex ] ) );
			}
		}
	}

	void cpu_topo::create_domain_masks()
	{
		unsigned	index{}, nxt_index{}, prev_bit{}, domains{ ( unsigned ) abl.size() };
		const auto &ca{ abl };
		for ( ; index < domains; ++index )
		{
			abl[ index ].relative_masks.emplace( index, ~( ( 1 << prev_bit ) - 1 ) );
			prev_bit = ca[ index ].shift;
		}
		for ( index = 0u; index < domains; ++index )
			for ( nxt_index = index + 1; nxt_index <= domains; ++nxt_index )
				abl[ index ].relative_masks.emplace(
					nxt_index, ( ~ca[ nxt_index ].relative_masks[ nxt_index ] )
								   & ( ca[ index ].relative_masks[ index ] ) );
	}

	unsigned int cpu_topo::CreateTopologyShift( unsigned int count )
	{
		unsigned int Shift{ 31u };
		unsigned int Index{ ( 1u << Shift ) };

		count = ( count * 2 ) - 1;
		for ( ; Index; Index >>= 1, Shift-- )
			if ( count & Index ) break;

		return Shift;
	}

	void cpu_topo::PopulatePlatformApicIds( cpuid_result &CpuidRegisters )
	{
		// Determine X2APIC ID or fall back to APIC ID.
		auto NumberOfProcessors{ get_logical_cpu_count() };
		auto id{ std::min( CpuidRegisters.x.Register.Eax, 0x1Fu ) };

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

	void cpu_topo::ThreeDomainFinalize( unsigned int PackageShift,
										unsigned int LogicalProcessorShift )
	{
		lvl_ids.resize( 3 );
		unsigned int LogicalProcessorMask{ ( 1u << LogicalProcessorShift ) - 1 };
		unsigned int CoreProcessorMask( ( ( 1u << PackageShift ) - 1 ) ^ LogicalProcessorMask );
		unsigned int PackageMask{ ~( ( 1u << PackageShift ) - 1 ) };

		unsigned int NumberOfLogicalProcessors{ ( unsigned ) apic_cpu_ids.size() };
		for ( unsigned int ProcessorIndex = 0; ProcessorIndex < NumberOfLogicalProcessors;
			  ProcessorIndex++ )
		{
			id_list id( { apic_cpu_ids[ ProcessorIndex ],
						  apic_cpu_ids[ ProcessorIndex ] >> LogicalProcessorShift } );
			lvl_ids[ 0 ][ id[ 0 ] ]++;
			lvl_ids[ 1 ][ id[ 1 ] ]++;
			lvl_ids[ 2 ][ 0 ]++;
			cpu_ids.emplace_back( make_pair( apic_cpu_ids[ ProcessorIndex ], move( id ) ) );
		}
	}

	void cpu_topo::ManyDomainFinalize( APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx )
	{
		unsigned int DomainShift;
		unsigned int NumberOfLogicalProcessors{ ( unsigned ) apic_cpu_ids.size() };
		unsigned int TopDomainIndex{ pApicidBitLayoutCtx->PackageDomainIndex };
		unsigned int ProcessorIndex{ 0 };
		unsigned int DomainIndex{ 0 };
		// produce topology masks depending on what we got
		for ( ; DomainIndex <= TopDomainIndex; DomainIndex++ )
			if ( pApicidBitLayoutCtx->ShiftValues[ DomainIndex ] != 0 )
				level_masks_names.emplace(
					pApicidBitLayoutCtx->ShiftValueDomain[ DomainIndex ],
					make_pair(
						pApicidBitLayoutCtx->DomainRelativeMasks[ DomainIndex ][ DomainIndex ],
						DomainIndex == TopDomainIndex
							? "_pkg_"
							: lvl_base_names[ pApicidBitLayoutCtx
												  ->ShiftValueDomain[ DomainIndex ] ] ) );

		lvl_ids.resize( TopDomainIndex + 1 );
		for ( ; ProcessorIndex < apic_cpu_ids.size(); ProcessorIndex++ )
		{
			id_list id;
			for ( DomainIndex = 0, DomainShift = 0; DomainIndex < TopDomainIndex; DomainIndex++ )
			{
				if ( pApicidBitLayoutCtx->ShiftValues[ DomainIndex ] != 0 )
				{
					const auto index =
						( pApicidBitLayoutCtx->DomainRelativeMasks[ DomainIndex ][ TopDomainIndex ]
						  & apic_cpu_ids[ ProcessorIndex ] )
						>> DomainShift;
					lvl_ids[ DomainIndex ][ index ]++;
					id.push_back( index );
				}
				DomainShift = pApicidBitLayoutCtx->ShiftValues[ DomainIndex ];
			}
			lvl_ids[ DomainIndex ]
				   [ ( pApicidBitLayoutCtx->DomainRelativeMasks[ TopDomainIndex ][ TopDomainIndex ]
					   & apic_cpu_ids[ ProcessorIndex ] )
					 >> pApicidBitLayoutCtx->ShiftValues[ TopDomainIndex - 1 ] ]++;
			cpu_ids.emplace_back( make_pair( apic_cpu_ids[ ProcessorIndex ], id ) );
		}
	}

	void cpu_topo::finish_topology()
	{
		unsigned int top_domain{ ( unsigned ) abl.size() };
		unsigned int cpu_cnt{ ( unsigned ) apic_cpu_ids.size() };
		unsigned int domain{ 0 };
		unsigned int domain_shift;
		// produce topology masks depending on what we got
		for ( ; domain <= top_domain; domain++ )
			if ( abl[ domain ].shift != 0 )
				level_masks_names.emplace(
					abl[ domain ].shift,
					make_pair( abl[ domain ].relative_masks[ domain ],
							   domain == top_domain ? "_pkg_"
													: lvl_base_names[ abl[ domain ].domain ] ) );

		lvl_ids.resize( top_domain + 1 );
		for ( unsigned cpu{}; cpu < cpu_cnt; cpu++ )
		{
			id_list id;
			for ( domain = 0, domain_shift = 0; domain < top_domain; domain++ )
			{
				if ( abl[ domain ].shift != 0 )
				{
					const auto index =
						( abl[ domain ].relative_masks[ top_domain ] & apic_cpu_ids[ cpu ] )
						>> domain_shift;
					lvl_ids[ domain ][ index ]++;
					id.push_back( index );
				}
				domain_shift = abl[ domain ].shift;
			}
			lvl_ids[ domain ]
				   [ ( abl[ top_domain ].relative_masks[ top_domain ] & apic_cpu_ids[ cpu ] )
					 >> abl[ top_domain - 1 ].shift ]++;
			cpu_ids.emplace_back( make_pair( apic_cpu_ids[ cpu ], id ) );
		}
	}

	pair< string, string > cpu_topo::getFmt() const noexcept
	{
		const auto sz{ 1 + ( unsigned int ) std::log10( get_logical_cpu_count() ) };
		return { '{' + std::format( ":0{:d}d", sz ) + '}',
				 '{' + std::format( ":#0{:d}X", sz + 2 ) + '}' };
	}
	id_list optimalProcessAffinity( int thread_count, bool prefer_performance, cpu_topo topology )
	{
		id_list ids;
		// prefer_performance means cores, that do not share logical CPUs
		if ( prefer_performance )
		{
		} else
		{}
		return ids;
	}
	void setThreadAffinity( const id_list &target_ids ) {}
} // namespace cpu_info