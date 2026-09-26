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

#ifdef _WIN32
#	define NOMINMAX
#	include <Windows.h>
#endif

#include <cpu_id.h>

#ifdef linux
#	include <sched.h>
#	include <unistd.h>
#	include <sys/sysinfo.h>
#endif

#include <set>
#include <ranges>
#include <format>

namespace cpu_info
{
	/* static */
	cpuid_result call_cpuid( unsigned int Leaf, unsigned int Subleaf ) noexcept
	{
		cpuid_result CpuidRegisters{};
#if _WIN32
		__cpuidex( reinterpret_cast< int * >( &CpuidRegisters.r[ 0 ] ), Leaf, Subleaf );

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

		CpuidRegisters.e.ax = ReturnEax;
		CpuidRegisters.e.bx = ReturnEbx;
		CpuidRegisters.e.cx = ReturnEcx;
		CpuidRegisters.e.dx = ReturnEdx;
#else
#endif
		return CpuidRegisters;
	}

#pragma region cpuid_leafs ... collection of cpuid data
	/*
	 *	cpuid_leafs enumerate all necessary leafs and subleafs on creation,
	 *	so we do not have to switch process affinity all the time.
	 */
	cpuid_leafs::L cpuid_leafs::_invalid{};
	cpuid_leafs::cpuid_leafs()
		: M()
		, _maxLeaf( emplace( 0u, L{ call_cpuid( 0, 0 ) } ).first->second.front().e.ax )
	{
		// create the CPUID-LEAFS map:
		for ( unsigned leaf: views::iota( 0u, _maxLeaf ) ) retrieve( leaf + 1 );
	}

	/*
	 *	The following function is modeled after INTEL's documentation.  As the CTor calls tries
	 *	to build a list of all cpuid_results available on this logical core, it needs to know
	 *	which restrictions apply and how to know which sub-pages are available / valid.
	 */
	cpuid_leafs::L &cpuid_leafs::retrieve( unsigned leaf ) noexcept
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
		if ( leaf > _maxLeaf || reserved.contains( leaf ) ) return _invalid;
		if ( requirements.contains( leaf ) ) // need a check
		{
			const auto &req = requirements.at( leaf );
			if ( at( req >> 24 ).at( ( req >> 16 ) & 0xff ).r[ ( req >> 8 ) & 3 ]
				 & ( 1 << ( req & 0x1f ) ) == 0 )
				return _invalid;
		}
		// ok, so the leaf should be available / valid …
		auto l = emplace( leaf, L{ call_cpuid( leaf, 0 ) } ).first;
		switch ( leaf )
		{
			case 0x04: // leaf #04 reports 0 within eax[4:0] on the last leaf.
				while ( l->second.back().e.ax & 0x1f )
					l->second.emplace_back( move( call_cpuid( leaf, l->second.size() ) ) );
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
						l->second.emplace_back( move( call_cpuid( leaf, s ) ) );
				break;
			case 0x0a: // This leaf is valid if CPUID.0AH:EAX[7:0] (Version ID) > 0
				if ( l->second.front().e.ax & 0xff ) break;
				erase( l ); // make bad in case of invalid result
				return _invalid;
			case 0x0b: // leaf #0b reports 0 within ebx[15:0] on the last leaf.
			case 0x1f: // leaf #1f reports 0 within ebx[15:0] on the last leaf.
				while ( l->second.back().e.bx & 0xffff )
					l->second.emplace_back( move( call_cpuid( leaf, l->second.size() ) ) );
				break;
			case 0x0d: // leaf #0d is special … subleafs 0 and 1 are always valid.
				l->second.emplace_back( move( call_cpuid( leaf, 1 ) ) );
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
						l->second.emplace_back( move( call_cpuid( leaf, subleaf ) ) );
					else if ( shifted ) // invalid, but valid leafs left?
						l->second.emplace_back();
					if ( ( shifted >> 1 ) == 0 ) break; // no more valid leafs
				}
				break;
			case 0x12: // subleafs 0 and 1 are always valid,
					   // Sub-leaf n (n ≥ 2) is only valid when CPUID.12H.n:EAX[3:0] != 0
				l->second.emplace_back( move( call_cpuid( leaf, 1 ) ) );
				do l->second.emplace_back( move( call_cpuid( leaf, l->second.size() ) ) );
				while ( l->second.back().e.ax & 0xf );
				l->second.pop_back();
				break;
			case 0x1b: // leaf #1b: Sub-leaf n is only valid when CPUID.1BH.n:EAX[11:0] != 0
				while ( l->second.back().e.ax & 0xFFF )
					l->second.emplace_back( move( call_cpuid( leaf, l->second.size() ) ) );
				l->second.pop_back();
				break;
		}
		return l->second;
	}

#pragma endregion

#pragma region cpu_id ... one such cpuid_leafs
	/* static */
	int		cpu_id::fmt_width{ 2 };

	cpu_id::operator apic_id() const noexcept
	{
		if ( _maxLeaf < 0x0b ) return at( 1 )[ 0 ].e.bx >> 24;
		else if ( _maxLeaf < 0x1f ) return at( 0xb )[ 0 ].e.dx;
		else return at( 0x1f )[ 0 ].e.dx;
	}

	cpu_id::operator string() const noexcept
	{
		string list, fmtstr = '{' + format( ":#0{:d}X", fmt_width + 2 ) + '}';
		for ( auto i( 0u ); i < masked_ids.size(); ++i )
		{
			if ( !list.empty() ) list.insert( list.begin(), ':' );
			list.insert( 0, vformat( fmtstr, make_format_args( masked_ids[ i ] ) ) );
		}
		const unsigned ai = apic_id( *this );
		return vformat( fmtstr + ":({:s})", make_format_args( ai, list ) );
	}

	bool cpu_id::operator()( cpu_feature feature ) const
	{
		const auto f = ( unsigned ) feature;
		const auto l = get_leaf( feature );
		const auto s = get_subleaf( feature );
		const auto r = get_register( feature );
		const auto b = get_bit( feature );
		if ( contains( l ) && at( l ).size() > s ) return ( at( l )[ s ].r[ r ] >> b ) & 1;
		else return false;
	}

	uint8_t cpu_id::stepping() const noexcept
	{ return at( 1 )[ 0 ].e.ax & 0b1111; }

	uint8_t cpu_id::family() const noexcept
	{
		auto fid{ uint8_t( at( 1 )[ 0 ].e.ax >> 8 ) & 0b1111 };
		if ( fid != 0x0f ) return fid;
		else return fid + ( ( at( 1 )[ 0 ].e.ax >> 20 ) & 0xff );
	}

	uint8_t cpu_id::model() const noexcept
	{
		auto fid{ uint8_t( at( 1 )[ 0 ].e.ax >> 8 ) & 0b1111 };
		auto mid{ uint8_t( at( 1 )[ 0 ].e.ax >> 4 ) & 0b1111 };
		if ( fid != 6 && fid != 15 ) return mid;
		return mid + ( uint8_t( at( 1 )[ 0 ].e.ax >> ( 16 - 4 ) ) & ~0b1111 );
	}

	cpu_processor_type cpu_id::type() const noexcept
	{ return static_cast< cpu_processor_type >( ( at( 1 )[ 0 ].e.ax >> 12 ) & 0b11 ); }

	cpu_core_type cpu_id::coreType() const noexcept
	{
		if ( size() >= 0x1a ) return cpu_core_type( at( 0x1a )[ 0 ].e.ax >> 24 );
		else return cpu_core_type::RESERVED1;
	}

	unsigned cpu_id::coreModel() const noexcept
	{
		if ( size() >= 0x1a ) return at( 0x1a )[ 0 ].e.ax & 0xffffff;
		else return 0u;
	}

#pragma endregion

} // namespace cpu_info