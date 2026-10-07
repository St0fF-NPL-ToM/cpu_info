#pragma once
/**
 * 	cpu_info:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a namespace with some functions
 *
 *  This "function namespace" version tries to simplify the whole thing a little.
 *
 *  There are a few caveats, though:
 *  ================================
 *  - a lot of features are the same over a module
 *    - e.g. the brand information (family, model, stepping, band_name)
 *    - many BUT NOT ALL features
 *  - but another load of them differs per core!
 *
 *	Thus, there is a need to be able to bind the calling thread to a specific core.
 *	With this need, resetting the thread affinity would be the second requirement.
 *	Thus, one could implement a kind of "iterator" …
 */
#ifdef _WIN32
	#define NOMINMAX
	#include <Windows.h>
#elifdef linux
	#include <sys/sysinfo.h>
	#include <unistd.h>
	#include <sched.h>
#endif

#include <cstdint>
#include <string>
#include <ranges>
#include <vector>
#include <functional>

namespace cpu_info
{

#pragma region INTEL keywords, flags and respective enumerations
	/*
	 *	The maximum number of enumerated domains, since X2APIC is 32 bits
	 *	there really can't be more than 32 domains enumerated.
	 */
	constexpr unsigned MAXIMUM_DOMAINS = 32;
	constexpr unsigned MAX_PROCESSORS  = 1024; // not sure if this is necessary!
	// clang-format off
	/**
	* 	enumeration of Intel-defined cpu capability flags userspace might want to test.
	*
	* 	to provide a clean nomenclature, let's assume some factors:
	*	- LEAF: (feat >> 8) & 0xff
	*	- SUB:	(feat >> 16) & 0xff
	*	- REG:	(feat >> 5) & 0x03
	*	- BIT:	(feat >> 0) & 0x1f
	*
	*	This makes up for an X-Macro-List ( NAME, REG, BIT, LEAF, SUB )
	*/
	#define CPU_FEATURES( X ) \
		X( SSE3, 2, 0, 1, 0 )				X( PCLMULQDQ, 2, 1, 1, 0 )	X( DTES64, 2, 2, 1, 0 )\
		X( MONITOR, 2, 3, 1, 0 )			X( DS_CPL, 2, 4, 1, 0 )		X( VMX, 2, 5, 1, 0 )\
		X( SMX, 2, 6, 1, 0 )				X( EIST, 2, 7, 1, 0 )		X( TM2, 2, 8, 1, 0 )\
		X( FMA, 2, 12, 1, 0 )				X( SSSE3, 2, 9, 1, 0 )		X( L1_CONTEXT_ID, 2, 10, 1, 0 )\
		X( DEBUG_INTERFACE, 2, 11, 1, 0 )	X( PCID, 2, 17, 1, 0 )		X( DCA, 2, 18, 1, 0 )\
		X( CMPXCHG16B, 2, 13, 1, 0 )		X( X2APIC, 2, 21, 1, 0 )	X( MOVBE, 2, 22, 1, 0 )\
		X( XTPR_UPDATE_CONTROL, 2, 14, 1, 0 )							X( SSE4_1, 2, 19, 1, 0 )\
		X( PERF_CAPABILITIES, 2, 15, 1, 0 )	X( SSE4_2, 2, 20, 1, 0 )	X( PBE, 3, 31, 1, 0 )\
		X( AESNI, 2, 25, 1, 0 )				X( POPCNT, 2, 23, 1, 0 )	X( TSC_DEADLINE, 2, 24, 1, 0 )\
		X( XSAVE, 2, 26, 1, 0 )				X( OSXSAVE, 2, 27, 1, 0 )	X( AVX, 2, 28, 1, 0 )\
		X( F16C, 2, 29, 1, 0 )				X( RDRAND, 2, 30, 1, 0 )	X( FPU, 3, 0, 1, 0 )\
		X( VME, 3, 1, 1, 0 )				X( DE, 3, 2, 1, 0 )			X( PSE, 3, 3, 1, 0 )\
		X( TSC, 3, 4, 1, 0 )				X( MSR, 3, 5, 1, 0 )		X( PAE, 3, 6, 1, 0 )\
		X( MCE, 3, 7, 1, 0 )				X( CMPXCHG8B, 3, 8, 1, 0 )	X( APIC, 3, 9, 1, 0 )\
		X( SEP, 3, 11, 1, 0 )				X( MTRR, 3, 12, 1, 0 )		X( PGE, 3, 13, 1, 0 )\
		X( MCA, 3, 14, 1, 0 )				X( CMOV, 3, 15, 1, 0 )		X( PAT, 3, 16, 1, 0 )\
		X( PSE_36, 3, 17, 1, 0 )			X( PSN, 3, 18, 1, 0 )		X( CLFLUSH, 3, 19, 1, 0 )\
		X( DS, 3, 21, 1, 0 )				X( ACPI, 3, 22, 1, 0 )		X( MMX, 3, 23, 1, 0 )\
		X( FXSR, 3, 24, 1, 0 )				X( SSE, 3, 25, 1, 0 )		X( SSE2, 3, 26, 1, 0 )\
		X( SELF_SNOOP, 3, 27, 1, 0 ) 		X( HTT, 3, 28, 1, 0 )		X( TM, 3, 29, 1, 0 )\
		/* Leaf 06H ThermalField Name */\
		X( DIGITAL_TEMP_SENSOR, 0, 0, 6, 0 )			X( TURBO_BOOST, 0, 1, 6, 0 )\
		X( ALWAYS_RUNNING_APIC_TIMER, 0, 2, 6, 0 )		X( POWER_LIMIT_NOTIFY, 0, 4, 6, 0 )\
		X( HWP_ACTIVITY_WINDOW, 0, 9, 6, 0 )			X( PKG_THERM_MGMT, 0, 6, 6, 0 )\
		X( HWP_INTERRUPT, 0, 8, 6, 0 )	X( HWP, 0, 7, 6, 0 )	X( EXT_CLOCK_MOD, 0, 5, 6, 0 )\
		X( HWP_REQUEST_PKG, 0, 11, 6, 0 )	X( HWP_EPP, 0, 10, 6, 0 )	X( HDC, 0, 13, 6, 0 )\
		X( TURBO_BOOST_MAX, 0, 14, 6, 0 )				X( HWP_CAP, 0, 15, 6, 0 ) \
		X( HWP_PECI_OVERRIDE, 0, 16, 6, 0 )				X( FLEXIBLE_HWP, 0, 17, 6, 0 )\
		X( HWP_REQUEST_FAST_ACCESS, 0, 18, 6, 0 )		X( HW_FEEDBACK, 0, 19, 6, 0 )\
		X( HWP_REQUEST_IGNORE_IDLE, 0, 20, 6, 0 )		X( HWP_CTL, 0, 22, 6, 0 )\
		X( THREAD_DIRECTOR, 0, 23, 6, 0 )				X( HW_FEEDBACK_CAP, 2, 0, 6, 0 )\
		X( ENERGY_PERF_BIAS, 2, 3, 6, 0 )\
		/* Leaf 07H Structured Extended Feature Flags */\
		X( FSGSBASE, 1, 0, 7, 0)	X( TSC_ADJUST, 1, 1, 7, 0)	X( SGX, 1, 2, 7, 0)	X( BMI1, 1, 3, 7, 0)\
		X( HLE, 1, 4, 7, 0)			X( AVX2, 1, 5, 7, 0)		X( FDP_EXCPTN_ONLY, 1, 6, 7, 0 )\
		X( SMEP, 1, 7, 7, 0 )		X( BMI2, 1, 8, 7, 0 )		X( ENH_REP_MOVSB_STOSB, 1, 9, 7, 0 )\
		X( INVPCID, 1, 10, 7, 0 )	X( RTM, 1, 11, 7, 0 )		X( RDT_M, 1, 12, 7, 0 )\
		X( FCS_FDS_DEPRECATION, 1, 13, 7, 0 )	X( MPX, 1, 14, 7, 0 )	X( RDT_A, 1, 15, 7, 0 )\
		X( AVX512F, 1, 16, 7, 0 )	X( AVX512DQ, 1, 17, 7, 0 )	X( RDSEED, 1, 18, 7, 0 )\
		X( ADX, 1, 19, 7, 0 )		X( SMAP, 1, 20, 7, 0 )		X( AVX512_IFMA, 1, 21, 7, 0 )\
		X( CLFLUSHOPT, 1, 23, 7, 0 )	X( CLWB, 1, 24, 7, 0 )	X( INTEL_PROC_TRACE, 1, 25, 7, 0 )\
		X( AVX512PF, 1, 26, 7, 0 )	X( AVX512ER, 1, 27, 7, 0 )	X( AVX512CD, 1, 28, 7, 0 )\
		X( SHA, 1, 29, 7, 0 )		X( AVX512BW, 1, 30, 7, 0 )	X( AVX512VL, 1, 31, 7, 0 )\
		X( PREFETCHWT1, 2, 0, 7, 0 )	X( AVX512_VBMI, 2, 1, 7, 0 )	X( UMIP, 2, 2, 7, 0 )\
		X( PKU, 2, 3, 7, 0 )		X( OSPKE, 2, 4, 7, 0 )		X( WAITPKG, 2, 5, 7, 0 )\
		X( AVX512_VBMI2, 2, 6, 7, 0 )	X( CET_SS, 2, 7, 7, 0 )	X( GFNI, 2, 8, 7, 0 )\
		X( VAES, 2, 9, 7, 0 )		X( VPCLMULQDQ, 2, 10, 7, 0 )	X( AVX512_VNNI, 2, 11, 7, 0 )\
		X( AVX512_BITALG, 2, 12, 7, 0 )	X( TME_EN, 2, 13, 7, 0 )	X( AVX512_VPOPCNTDQ, 2, 14, 7, 0 )\
		X( LA57, 2, 16, 7, 0 )		X( RDPID, 2, 22, 7, 0 )		X( KEY_LOCKER, 2, 23, 7, 0 )\
		X( BUS_LOCK_DETECT, 2, 24, 7, 0 )	X( CLDEMOTE, 2, 25, 7, 0 )	X( MOVDIRI, 2, 27, 7, 0 )\
		X( MOVDIR64B, 2, 28, 7, 0 )	X( ENQCMD, 2, 29, 7, 0 )	X( SGX_LC, 2, 30, 7, 0 )\
		X( PKS, 2, 31, 7, 0 )		X( SGX_KEYS, 3, 1, 7, 0 )	X( AVX512_4VNNIW, 3, 2, 7, 0 )\
		X( UINTR, 3, 5, 7, 0 )	X( AVX512_4FMAPS, 3, 3, 7, 0 )	X( FAST_SHORT_REP_MOVSB, 3, 4, 7, 0 )\
		X( MD_CLEAR, 3, 10, 7, 0 )	X( AVX512_VP2INTERSECT, 3, 8, 7, 0 )	X( MCU_OPT_CTRL, 3, 9, 7, 0 )\
		X( RTM_ALWAYS_ABORT, 3, 11, 7, 0 )		X( RTM_FORCE_ABORT, 3, 13, 7, 0 )\
		X( SERIALIZE, 3, 14, 7, 0 )	X( HYBRID, 3, 15, 7, 0 )	X( TSXLDTRK, 3, 16, 7, 0 )\
		X( PCONFIG, 3, 18, 7, 0 )	X( ARCH_LBRS, 3, 19, 7, 0 )	X( CET_IBT, 3, 20, 7, 0 )\
		X( AMX_BF16, 3, 22, 7, 0 )	X( AVX512_FP16, 3, 23, 7, 0 )\
		X( AMX_TILE, 3, 24, 7, 0 )	X( AMX_INT8, 3, 25, 7, 0 )	X( IBRS_IBPB, 3, 26, 7, 0 )\
		X( SPEC_CTRL_ST_PREDICTORS, 3, 27, 7, 0 )				X( L1D_FLUSH_INTERFACE, 3, 28, 7, 0 )\
		X( ARCH_CAPABILITIES, 3, 29, 7, 0 )						X( CORE_CAPABILITIES, 3, 30, 7, 0 )\
		X( SPEC_CTRL_SSBD, 3, 31, 7, 0 )\
		/* Leaf 07H.01H StructuredField Name */\
		X( SHA512, 0, 0, 7, 1 )		X( SM3, 0, 1, 7, 1 )				X( SM4, 0, 2, 7, 1 )\
		X( AVX_VNNI, 0, 4, 7, 1 )	X( AVX512_BF16, 0, 5, 7, 1 )		X( LASS, 0, 6, 7, 1 )\
		X( CMPCCXADD, 0, 7, 7, 1 )	X( ARCH_PERFMON_EXT, 0, 8, 7, 1 )	X( FAST_REP_MOVSB, 0, 10, 7, 1 )\
		X( FAST_REP_STOSB, 0, 11, 7, 1 )				X( FAST_REP_CMPSB_SCASB, 0, 12, 7, 1 )\
		X( FRED, 0, 17, 7, 1 )		X( LKGS, 0, 18, 7, 1 )				X( WRMSRNS, 0, 19, 7, 1 )\
		X( NMI_SRC, 0, 20, 7, 1 )	X( AMX_FP16, 0, 21, 7, 1 )			X( HRESET, 0, 22, 7, 1 )\
		X( AVX_IFMA, 0, 23, 7, 1 )	X( LAM, 0, 26, 7, 1 )				X( MSRLIST, 0, 27, 7, 1 )\
		X( INVD_DISABLE_POST_BIOS_DONE, 0, 30, 7, 1 )					X( PPIN, 1, 0, 7, 1 )\
		X( PBNDKB, 1, 1, 7, 1 )		X( CPUIDMAXVAL_LIM_RMV, 1, 3, 7, 1 )\
		X( RDT_M_ASYM, 2, 0, 7, 1 )	X( RDT_A_ASYM, 2, 1, 7, 1 )			X( MSR_IMM, 2, 5, 7, 1 )\
		X( AVX_VNNI_INT8, 3, 4, 7, 1 )	X( AVX_NE_CONVERT, 3, 5, 7, 1 ) X( AMX_COMPLEX, 3, 8, 7, 1 )\
		X( AVX_VNNI_INT16, 3, 10, 7, 1 )	X( PREFETCHI, 3, 14, 7, 1 )	X( USER_MSR, 3, 15, 7, 1 )\
		X( UIRET_UIF, 3, 17, 7, 1 )	X( CET_SSS, 3, 18, 7, 1 )			X( AVX10, 3, 19, 7, 1 )\
		X( SEC_TEE_ATTESTATION, 3, 22, 7, 1 )	X( MWAIT, 3, 23, 7, 1 )	X( SLSM, 3, 24, 7, 1 )\
		/* CPUID.07H.02 */\
		X( PSFD, 3, 0, 7, 2 )	X( IPRED_CTRL, 3, 1, 7, 2 )	X( RRSBA_CTRL, 3, 2, 7, 2 )\
		X( DDPD_U, 3, 3, 7, 2 )	X( BHI_CTRL, 3, 4, 7, 2 )	X( MCDT_NO, 3, 5, 7, 2 )\
		X( UC_LOCK_DISABLE, 3, 6, 7, 2 )					X( MONITOR_MITG_NO, 3, 7, 7, 2 )

	#define X( NAME, REG, BIT, LEAF, SUB ) NAME = ( SUB << 16 ) + ( LEAF << 8 ) + ( REG << 5 ) + BIT,
	enum class cpu_feature : unsigned { CPU_FEATURES( X ) };
	#undef X
	constexpr unsigned get_leaf( cpu_feature feat ) 		{ return ( ( ( unsigned ) feat ) >> 8 ) & 0xff; }
	constexpr unsigned get_subleaf( cpu_feature feat )		{ return ( ( ( unsigned ) feat ) >> 16 ) & 0xff; }
	constexpr unsigned get_register( cpu_feature feat )		{ return ( ( ( unsigned ) feat ) >> 5 ) & 0x3; }
	constexpr unsigned get_bit( cpu_feature feat )			{ return ( ( unsigned ) feat ) & 0x1f; }
	/*
	 * The enumeration of domain identifiers as specified by CPUID.1F and CPUID.B documentation.
	 *
	 * Using an X-macro driven approach, here …	again
	 */
	#define CPU_DOMAINS( X )	X( InvalidDomain )	X( LogicalDomain )	X( CoreDomain ) \
			X( ModuleDomain )	X( TileDomain )		X( DieDomain )		X( DieGrpDomain )
	#define X( name ) name,
	enum cpu_domain { CPU_DOMAINS( X ) };
	/*
	 *	PROCESSOR_TYPE field → has names …
	 */
	#define PROCESSOR_TYPE( X )	X( OEM_processor ) X( IntelOverDrive ) X( Dual_processor ) X( Intel_reserved )
	enum cpu_processor_type { PROCESSOR_TYPE( X ) };
	#undef X
	/*
	 *	CORE_TYPE field
	 */
	#define CORE_TYPE( X ) X( DUNNO, 0 ) X( RESRV, 0x10 ) X( AtomR, 0x20 ) X( R3SRV, 0x30 ) X( CoreI, 0x40 )
	#define X( name, n ) name = n,
	enum cpu_core_type { CORE_TYPE( X ) };
	/*
	 *	Regarding this field … there are performance- and efficiency-cores on some platforms.
	 *	I could not find out, what they really use (except for ARM littleBIG) and obviously
	 *	core_type::Atom vs. core_type::Core.
	 */
	#define EFFICIENCY_TYPE( X ) X( unknownEff, 0 ) X( effficient, 1 ) X( performant, 2 )
	enum cpu_efficiency { EFFICIENCY_TYPE( X ) };
	#undef X
	// clang-format on
#pragma endregion

#pragma region CPUID instruction, query logical cpu count - platform independent implementation
	// data types and namespace inclusions
	using namespace std;

	using APIC_id = unsigned;
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

	// execute the cpuid instruction
	inline cpuid_result cpuid( unsigned Leaf, unsigned Subleaf ) noexcept
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

	// query available logical cpus
	inline int count() noexcept
	{
#ifdef _WIN32
		PROCESSOR_NUMBER pn{};
		GetCurrentProcessorNumberEx( &pn );
		return GetActiveProcessorCount( pn.Group );
#elifdef linux
		return get_nprocs();
#endif
	}
#pragma endregion

#pragma region QUERY functions accessing current cpu

	inline unsigned id_leaf() noexcept // query the ID-leaf
	{
		const auto ml = cpuid( 0, 0 );
		return ml.e.ax ? ml.e.ax < 0x1f ? ml.e.ax < 0x0b ? 1 : 0x0b : 0x1f : 0;
	}

	inline bool has_feature( cpu_feature feature ) noexcept // check a specific feature
	{
		const auto l = get_leaf( feature );
		const auto s = get_subleaf( feature );
		const auto r = get_register( feature );
		const auto b = get_bit( feature );
		if ( l > 0 && l > id_leaf() ) return false;
		else return ( cpuid( l, s ).r[ r ] >> b ) & 1;
	}

	inline int domain_shift( cpu_domain domain ) noexcept // query domain shift
	{
		constexpr auto create_topology_shift = []( unsigned int count ) -> unsigned
		{
			unsigned int Shift{ 31u };
			unsigned int Index{ ( 1u << Shift ) };
			count = ( count * 2 ) - 1;
			for ( ; Index; Index >>= 1, Shift-- )
				if ( count & Index ) break;
			return Shift;
		};

		const auto ml = cpuid( 0, 0 );
		if ( ml.e.ax <= 1 ) // catch "illegal object" as "don't know anything"
		{
			if ( !has_feature( cpu_feature::HTT ) ) return create_topology_shift( 1 );
			else // max_leaf minimum = 1
			{
				const auto MaxIdsPhysical = ( unsigned ) ( ( cpuid( 1, 0 ).e.bx >> 16 ) & 0xFF );
				// This would be a 20+ year old platform to not support CPUID.4 … You cannot report
				// Cores here, a Package == Core and so this only reports SMT within a Package.
				if ( ml.e.ax < 4 || domain != cpu_domain::LogicalDomain )
					return create_topology_shift( MaxIdsPhysical );
				else /* MaximumAddressibleIdsCores: CPUID.4.0.EAX[31:26] */
					return create_topology_shift( MaxIdsPhysical
												  / ( ( cpuid( 4, 0 ).e.ax >> 26 ) + 1 ) );
			}
		} else
		{
			unsigned il = ml.e.ax < 0x1f ? 0x0b : 0x1f;
			unsigned sub{ 0 };
			auto	 sl = cpuid( il, sub );
			for ( ; sl.e.bx != 0; ++sub )
			{
				if ( sub ) sl = cpuid( il, sub );
				// CPUID.B or 1F.x.ECX[15:8] = Level Type / Domain Type
				// CPUID.B or 1F.x.EAX[4:0] = Level Shift / Domain Shift
				if ( domain == ( ( sl.e.cx >> 8 ) & 0xFF ) ) return sl.e.ax & 0x1F;
			}
			return sl.e.ax & 0x1F; // Fallback: top level shift propagates further …
		}
	}

	inline id_mask domain_mask( cpu_domain domain ) noexcept // query domain mask
	{
		if ( domain <= cpu_domain::LogicalDomain ) return -1u;
		// was ist nochmal die korrekte Domain Mask?  Bedeutet ja, dass 0-basierte Indices
		// rauskommen! Logical/Invalid: 0b1111… komplette ID Core: 	shift( Logical )= 1	→ in meinem
		// Falle: 0b01111110 Module: 	shift( Core ) 	= 7
		const auto ps = domain_shift( cpu_domain( domain - 1 ) );
		const auto ds = domain_shift( domain );
		return ( ( 1 << ds ) - 1 ) ^ ( ( 1 << ps ) - 1 );
	}

	inline APIC_id // produce apic_id - default logical domain is complete and unmasked!
	apic_id( cpu_domain domain = cpu_domain::LogicalDomain ) noexcept
	{
		const auto sl = id_leaf(); // retrieve APIC_ID
		const auto id = ( sl > 1 ) ? cpuid( sl, 0 ).e.dx
						: sl	   ? cpuid( 1, 0 ).e.bx >> 24
								   : -1u; // illegal!
										  // produce masked and shifted value
		return ( id & domain_mask( domain ) ) >> domain_shift( cpu_domain( domain - 1 ) );
	}

	inline uint8_t stepping() noexcept
	{ return cpuid( 1, 0 ).e.ax & 0b1111; }

	inline uint8_t family() noexcept
	{
		const auto l = cpuid( 1, 0 );
		const auto fid{ uint8_t( l.e.ax >> 8 ) & 0b1111 };
		return ( fid != 0x0f ? fid : fid + ( ( l.e.ax >> 20 ) & 0xff ) );
	}

	inline uint8_t model() noexcept
	{
		const auto l = cpuid( 1, 0 );
		auto	   fid{ uint8_t( l.e.ax >> 8 ) & 0b1111 };
		auto	   mid{ uint8_t( l.e.ax >> 4 ) & 0b1111 };
		return ( fid == 6 || fid == 15 ) ? mid | ( uint8_t( l.e.ax >> ( 16 - 4 ) ) & ~0b1111 )
										 : mid;
	}

	inline cpu_processor_type type() noexcept
	{ return static_cast< cpu_processor_type >( ( cpuid( 1, 0 ).e.ax >> 12 ) & 0b11 ); }

	inline cpu_core_type core_type() noexcept
	{
		if ( cpuid( 0, 0 ).e.ax >= 0x1a ) return cpu_core_type( cpuid( 0x1a, 0 ).e.ax >> 24 );
		else return cpu_core_type::DUNNO;
	}

	inline unsigned core_model() noexcept
	{
		if ( cpuid( 0, 0 ).e.ax >= 0x1a ) return cpu_core_type( cpuid( 0x1a, 0 ).e.ax & 0xffffff );
		else return 0u;
	}

	inline string brand_string() noexcept
	{
		constexpr auto		  ext_index			   = 0x80000000u;
		constexpr auto		  brand_string_support = 0x80000004u;
		constexpr const char *brand_strings[]{
			"Intel® Celeron®",
			"Intel® Pentium® III",
			"Intel® Pentium® III Xeon®",
			"Intel® Pentium® M",
			"Mobile Intel® Pentium® III-M",
			"Mobile Intel® Celeron®",
			"Intel® Pentium® 4",
			"Intel® Xeon®",
			"Intel® Xeon® MP",
			"Mobile Intel® Pentium® 4-M",
			"Mobile Genuine Intel®",
			"Intel® Celeron® M",
		}; // some of Intel®'s strings were double-defined or reserved/empty. So this is a
		   // remapping:
		constexpr unsigned brand_reindex[] = { 0x00, 0x01, 0x02, 0x01, 0,	 0x04, 0x05, 0x06,
											   0x06, 0x00, 0x07, 0x08, 0,	 0x09, 0x05, 0,
											   0x0a, 0x0b, 0x05, 0x00, 0x0a, 0x03, 0x05 };

		string			   result;
		if ( const auto sup = cpuid( ext_index, 0u ); sup.e.ax >= brand_string_support )
		{ // use brand string method
			result.resize( 4 * 4 * 3 + 1 );
			auto *p = reinterpret_cast< unsigned * >( result.data() );
			for ( auto s{ ext_index + 2 }; s <= brand_string_support; ++s )
			{
				const auto x = cpuid( s, 0u );
				for ( auto i: views::iota( 0, 4 ) ) *p++ = x.r[ i ];
			}
		} else if ( auto l1 = cpuid( 1, 0 ); auto brand_index = std::min( 0x17u, l1.e.bx & 0xff ) )
		{ // use middle-aged brand index method

			unsigned mf	 = uint8_t( l1.e.ax >> 8 ) & 0b1111; // need only leaf 1 - so,
			unsigned mid = uint8_t( l1.e.ax >> 4 ) & 0b1111; // why call it multiple times?
			if ( mf == 0x600 || mf == 0xf00 ) mid |= ( uint8_t( l1.e.ax >> ( 16 - 4 ) ) & ~0b1111 );
			mf = ( ( mf != 0x0f ? mf : mf + ( ( l1.e.ax >> 20 ) & 0xff ) ) << 8 ) | mid;
			switch ( mf )
			{
				case 0x6b1:
					if ( brand_index == 3 ) brand_index = 1;
					break;
				case 0xF13:
					if ( brand_index == 0xb ) brand_index = 0xc;
					if ( brand_index == 0xe ) brand_index = 0xb;
					break;
			}
			return { brand_strings[ brand_reindex[ brand_index - 1 ] ] };
		}
		return result;
	}

	inline cpu_efficiency efficiency() noexcept
	{
		if ( has_feature( cpu_feature::HYBRID ) ) // already checks leaf #0 for max leaf
			return ( ( cpuid( 0x1a, 0 ).e.ax & 0x70000000u ) > 0x20000000u ? performant
																		   : effficient );
		else return unknownEff;
	}

#pragma endregion

#pragma region AFFINITY helpers in a platform independent fashion

	/**
	 *	Original idea: provide some kind of "selected cpu iterator"
	 *	Problem of that: IT HIDES VERY USEFUL CODE!
	 *
	 *	Approach:
	 *	-	`affinity` as an object to describe process or thread affinity in a platform
	 *		independent manner.
	 *	-	a simple bitset, a group number and for systems that support it, a special
	 *		preferred cpu number.
	 */
	class affinity final
	{
		std::vector< bool > bit_mask;
		unsigned			group_id{ 0 };
		unsigned			pref_cpu{ std::numeric_limits< unsigned >::max() };

	  public:
		affinity() = default; // create empty
		affinity( int n )	  // create with one cpu-number selected
			: bit_mask( n, false )
			, pref_cpu( n )
		{ bit_mask.push_back( true ); }
		// inherit is to be used to copy from this, but select only one valid cpu
		affinity inherit( unsigned single_cpu )
		{ // e.g. group id from this, pref_cpu and mask from the other.
			affinity result( single_cpu );
			result.group_id = group_id;
			return result;
		}
		// check if all is empty / any bits are set
		operator bool() const noexcept
		{
			for ( auto b: bit_mask )
				if ( b ) return true;
			return false;
		}
		bool		empty() const noexcept { return !( *this ); }
		// return a string representation
					operator std::string() const noexcept { return to_string(); }
		std::string to_string() const noexcept
		{
			int			i( count() );
			std::string result( i, '-' );
			for ( const auto &b: bit_mask ) result[ --i ] = b ? '+' : '-';
			return result;
		}
		// count enabled items inside the mask
		size_t count_enabled() const noexcept
		{
			int en{};
			for ( auto b: bit_mask )
				if ( b ) ++en;
			return en;
		}

		// so it can be directly used after reading …
		affinity &read_current() noexcept { return query(), *this; }

		// set current thread affinity! USE WITH CARE!
		bool	  set_to_current() noexcept { return apply(); }

		// for single cpu-masks, this "shifts" the mask by one cpu and applies it
		bool	  apply_next() noexcept
		{
			// make sure it is a single mask
			if ( count_enabled() == 1 )
			{
				// remove 'false' elements from the back, so checking by size works.
				while ( !bit_mask.back() ) bit_mask.pop_back();
				// in case the last cpu number was active, we're done
				if ( bit_mask.size() != count() )
				{
					// shift mask by inserting an unused (e.g. 'false') bit
					bit_mask.insert( bit_mask.begin(), false );
					return apply();
				}
			}
			return false;
		}

		affinity // operate on two sets producing another one.
		op( const affinity &o, std::function< bool( bool, bool ) > operation ) const noexcept
		{
			affinity   res( *this );
			const auto sm{ bit_mask.size() }, so{ o.bit_mask.size() }, sz{ std::max( sm, so ) };
			res.bit_mask.resize( sz );
			for ( auto i: std::views::iota( 0ul, bit_mask.size() ) )
				res.bit_mask[ i ] = operation( ( i >= sm ? false : bit_mask[ i ] ),
											   ( i >= so ? false : o.bit_mask[ i ] ) );
			return res;
		} // clang-format off
		affinity operator|( const affinity &o ) const noexcept { return op( o, []( bool r, bool l ) { return ( r || l ); } ); }
		affinity operator&( const affinity &o ) const noexcept { return op( o, []( bool r, bool l ) { return ( r && l ); } ); }
		affinity operator^( const affinity &o ) const noexcept { return op( o, []( bool r, bool l ) { return ( r ^ l ); } ); }
		// clang-format on

		affinity &operator+=( const int cpu_number ) noexcept
		{
			if ( cpu_number < count() )
			{
				if ( cpu_number >= bit_mask.size() )
					bit_mask.resize( cpu_number, false ), bit_mask.push_back( true );
				else bit_mask[ cpu_number ] = true;
			}
			return *this;
		}

	  private:
#ifdef _WIN32
		bool query() noexcept
		{
			/*	Multiple ways lead to Rome … we need: a cpu-group id AND an affinity mask
			 *	→ easiest solution:
			 *		- GetCurrentProcessorNumberEx:	currently active cpu-group id
			 *		- GetProcessAffinityMask:		process affinity and system affinity
			 *		(logically, these affinities cover the previously acquired group id)
			 */
			PROCESSOR_NUMBER pn{};
			KAFFINITY		 pm{}, sa{};
			GetCurrentProcessorNumberEx( &pn );
			bool result = GetProcessAffinityMask( GetCurrentProcess(), &pm, &sa );
			group_id	= pn.Group;
			pref_cpu	= pn.Number;
			bit_mask.clear();
			while ( pm ) bit_mask.push_back( pm & 1 ), pm >>= 1;
			return result;
		}
		bool apply() noexcept
		{
			GROUP_AFFINITY ga{};
			ga.Group = group_id;
			for ( auto b: std::views::reverse( bit_mask ) )
				ga.Mask = ( KAFFINITY ) ( b | ( ga.Mask << 1 ) );
			bool result = SetThreadGroupAffinity( GetCurrentThread(), &ga, nullptr );
			if ( pref_cpu >= 0 ) SetThreadIdealProcessor( GetCurrentThread(), ( DWORD ) pref_cpu );
			return result;
		}
#elifdef linux
		bool query() noexcept
		{
			int		   sz{ count() };
			cpu_set_t *set{ CPU_ALLOC( sz ) };
			auto	   result = sched_getaffinity( getpid(), sz, set ) == 0;
			bit_mask.clear();
			for ( int n: std::views::iota( 0, sz ) )
				bit_mask.push_back( CPU_ISSET_S( n, sz, set ) );
			CPU_FREE( set );
			// i fear the linux cpu_set just has one group?
			return result;
		}
		bool apply() noexcept
		{
			int	  sz{ count() }, i{};
			auto *s = CPU_ALLOC( sz );
			CPU_ZERO_S( sz, s );
			for ( auto b: bit_mask )
				if ( b ) CPU_SET_S( i++, sz, s );
				else CPU_CLR_S( i++, sz, s );
			auto result = sched_setaffinity( getpid(), sz, s ) == 0;
			CPU_FREE( s );
			return result;
		}
#endif
	};

	/**
	 *	And for "retrieving info from all cpus" - here it is, the iterator.
	 *
	 *	Upon construction:
	 *	-	query current process affinity and remember it
	 *	-	produce a single-cpu-affinity
	 *	-	apply single-cpu-affinity
	 *	Upon increment:
	 *	-	switch to next cpu and return if end was reached
	 */
	class affinity_iterator final
	{
		affinity base, curr;

	  public:
		affinity_iterator() { base.read_current(), ( curr = base.inherit( 0 ) ).set_to_current(); }
		// in case the main thread was mistakenly not reset, it will be done upon leave.
		~affinity_iterator() { base.set_to_current(); }
		// switch to next logical core, return false when switching from the last core
		// back to the main thread base setting.
		bool operator++() noexcept
		{
			if ( curr.apply_next() ) return true;
			return base.set_to_current(), false;
		}
	};

#pragma endregion
} // namespace cpu_info
