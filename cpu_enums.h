/**
 * 	cpu_features:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_features enumeration
 */
#pragma once

namespace cpu_info
{
	/*
	 *	The maximum number of enumerated domains, since X2APIC is 32 bits
	 *	there really can't be more than 32 domains enumerated.
	 */
	constexpr unsigned MAXIMUM_DOMAINS = 32;
	constexpr unsigned MAX_PROCESSORS  = 1024; // not sure if this is necessary!

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
	// clang-format off
	#define CPU_FEATURES( X ) \
		X( SSE3, 2, 0, 1, 0 )		X( PCLMULQDQ, 2, 1, 1, 0 )	X( DTES64, 2, 2, 1, 0 )\
		X( MONITOR, 2, 3, 1, 0 )	X( DS_CPL, 2, 4, 1, 0 )		X( VMX, 2, 5, 1, 0 )\
		X( SMX, 2, 6, 1, 0 )		X( EIST, 2, 7, 1, 0 )		X( TM2, 2, 8, 1, 0 )\
		X( FMA, 2, 12, 1, 0 )		X( SSSE3, 2, 9, 1, 0 )		X( L1_CONTEXT_ID, 2, 10, 1, 0 )\
		X( DEBUG_INTERFACE, 2, 11, 1, 0 )	X( XTPR_UPDATE_CONTROL, 2, 14, 1, 0 )\
		X( CMPXCHG16B, 2, 13, 1, 0 )		X( PERF_CAPABILITIES, 2, 15, 1, 0 )\
		X( PCID, 2, 17, 1, 0 )		X( DCA, 2, 18, 1, 0 )		X( SSE4_1, 2, 19, 1, 0 )\
		X( SSE4_2, 2, 20, 1, 0 )	X( X2APIC, 2, 21, 1, 0 )	X( MOVBE, 2, 22, 1, 0 )\
		X( AESNI, 2, 25, 1, 0 )		X( POPCNT, 2, 23, 1, 0 )	X( TSC_DEADLINE, 2, 24, 1, 0 )\
		X( XSAVE, 2, 26, 1, 0 )		X( OSXSAVE, 2, 27, 1, 0 )	X( AVX, 2, 28, 1, 0 )\
		X( F16C, 2, 29, 1, 0 )		X( RDRAND, 2, 30, 1, 0 )	X( FPU, 3, 0, 1, 0 )\
		X( VME, 3, 1, 1, 0 )		X( DE, 3, 2, 1, 0 )			X( PSE, 3, 3, 1, 0 )\
		X( TSC, 3, 4, 1, 0 )		X( MSR, 3, 5, 1, 0 )		X( PAE, 3, 6, 1, 0 )\
		X( MCE, 3, 7, 1, 0 )		X( CMPXCHG8B, 3, 8, 1, 0 )	X( APIC, 3, 9, 1, 0 )\
		X( SEP, 3, 11, 1, 0 )		X( MTRR, 3, 12, 1, 0 )		X( PGE, 3, 13, 1, 0 )\
		X( MCA, 3, 14, 1, 0 )		X( CMOV, 3, 15, 1, 0 )		X( PAT, 3, 16, 1, 0 )\
		X( PSE_36, 3, 17, 1, 0 )	X( PSN, 3, 18, 1, 0 )		X( CLFLUSH, 3, 19, 1, 0 )\
		X( DS, 3, 21, 1, 0 )		X( ACPI, 3, 22, 1, 0 )		X( MMX, 3, 23, 1, 0 )\
		X( FXSR, 3, 24, 1, 0 )		X( SSE, 3, 25, 1, 0 )		X( SSE2, 3, 26, 1, 0 )\
		X( SELF_SNOOP, 3, 27, 1, 0 ) X( HTT, 3, 28, 1, 0 )		X( TM, 3, 29, 1, 0 )\
		X( PBE, 3, 31, 1, 0 ) \
		/* Leaf 06H ThermalField Name */\
		X( DIGITAL_TEMP_SENSOR, 0, 0, 6, 0 )			X( TURBO_BOOST, 0, 1, 6, 0 )				\
		X( ALWAYS_RUNNING_APIC_TIMER, 0, 2, 6, 0 )		X( POWER_LIMIT_NOTIFY, 0, 4, 6, 0 )			\
		X( HWP_ACTIVITY_WINDOW, 0, 9, 6, 0 )			X( PKG_THERM_MGMT, 0, 6, 6, 0 )				\
		X( HWP_INTERRUPT, 0, 8, 6, 0 )	X( HWP, 0, 7, 6, 0 )	X( EXT_CLOCK_MOD, 0, 5, 6, 0 )\
		X( HWP_REQUEST_PKG, 0, 11, 6, 0 )	X( HWP_EPP, 0, 10, 6, 0 )	X( HDC, 0, 13, 6, 0 )\
		X( TURBO_BOOST_MAX, 0, 14, 6, 0 )				X( HWP_CAP, 0, 15, 6, 0 ) \
		X( HWP_PECI_OVERRIDE, 0, 16, 6, 0 )				X( FLEXIBLE_HWP, 0, 17, 6, 0 ) \
		X( HWP_REQUEST_FAST_ACCESS, 0, 18, 6, 0 )		X( HW_FEEDBACK, 0, 19, 6, 0 ) \
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
	inline unsigned get_leaf( cpu_feature feat ) 		{ return ( ( ( unsigned ) feat ) >> 8 ) & 0xff; }
	inline unsigned get_subleaf( cpu_feature feat )		{ return ( ( ( unsigned ) feat ) >> 16 ) & 0xff; }
	inline unsigned get_register( cpu_feature feat )	{ return ( ( ( unsigned ) feat ) >> 5 ) & 0x3; }
	inline unsigned get_bit( cpu_feature feat )			{ return ( ( unsigned ) feat ) & 0x1f; }
	/*
	 * The enumeration of domain identifiers and these need to each match
	 * the value as specified by CPUID.1F and CPUID.B documentation.
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
	#define CORE_TYPE( X ) X( RESERVED1, 0x10 ) X( Atom, 0x20 ) X( RESERVED3, 0x30 ) X( CoreI, 0x40 )
	#define X( name, n ) name = n,
	enum cpu_core_type { CORE_TYPE( X ) };
	#undef X
	// clang-format on
} // namespace cpu_info