#pragma once
/**
 * 	cpu_topo:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
 *
 * 	Usage:
 * ========
 * 	namespace functions:
 * 		cpuid_result	call_cpuid( Leaf, Sub ) 		→ execute the respective CPUID
 *		void 			bind_thread_to_cpu( cpuNumber ) → what it's called …
 *		unsigned 		get_logical_cpu_count() 		→ again, the naming speaks …
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *	Implementation progress:
 * ==========================
 * 	Step #1:	simple cpu enumeration on a system with subclassing by CPU domains
 * 			- query count of CPUs of a domain
 * 			- retrieve (un)masked APIC IDs per cpu
 * 	→ 	Solves the question of "how many threads do make sense in certain scenarios"
 *		by comparing different level counts.
 *	→	[x] feature-complete!
 *
 * 	Step #2:	query (if available) core_types (efficiency/performance etc.)
 *
 * 	Step #3:	also query "memory"-items, so scoring by shared / non-shared ids becomes possible.
 *
 */

#include <vector>
#include <map>
#include <format>
#include <climits>
#include <cmath>

namespace cpu_info
{
	using namespace std;

	using apic_id = unsigned;
	using id_list = vector< apic_id >;
	using id_mask = unsigned;
	struct cpuid_result;

#pragma region interface functions

	/* static interface functions:
	 */
	extern cpuid_result call_cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;
	extern void			bind_thread_to_cpu( unsigned ProcessorNumber );
	extern unsigned		get_logical_cpu_count() noexcept;

	/*
	 *	Extra functions that may be helpful on larger systems
	 *	when using excessive threading in your application.
	 */
	extern void			setThreadAffinity( const id_list &target_ids );

#pragma endregion
#pragma region declare structures

	/**
	 * 	Result of a cpuid - instruction (simply 4 32bit registers)
	 */
	struct cpuid_result
	{
		union
		{
			unsigned int Registers[ 4 ];
			struct
			{
				unsigned int Eax;
				unsigned int Ebx;
				unsigned int Ecx;
				unsigned int Edx;
			} Register;
		} x;
	};

	/**
	 * 	enumeration of Intel-defined cpu capability flags userspace might want to test.
	 */
	enum class cpu_feature : unsigned { // clang-format off
		/* 	to provide a clean nomenclature, let's assume some factors:
		 *	- LEAF: (feat >> 8) & 0xff
		 *	- SUB:	(feat >> 16) & 0xff
		 *	- REG:	(feat >> 5) & 0x03
		 *	- BIT:	(feat >> 0) & 0x1f
		 */
		// CPUID.01
		#define C1( NAME, REG, BIT ) NAME = 0x0120 + REG * 32 + BIT
		C1( ACPI, 2, 22 ),	C1( MMX, 2, 23 ),	C1( SSE, 2, 25 ),	C1( SSE2, 2, 26 ),
		C1( SSE3, 1, 0 ),	C1( SSSE3, 1, 9 ),	C1( FMA, 1, 12 ),	C1( SSE41, 1, 19 ),
		C1( SSE42, 1, 20 ),	C1( AESNI, 1, 25 ),	C1( AVX, 1, 28 ),	C1( F16C, 1, 29 ),
		C1( HTT, 2, 28 ),	C1( TSC, 2, 4 ),	C1( TM, 2, 29 ),	C1( TM2, 1, 8 ),
		#undef C1
		#define CF( NAME, LEAF, SUB, REG, BIT ) NAME = (LEAF << 8) + (SUB << 16) + (REG * 32) + BIT
		// CPUID.07
		CF( AVX2, 7, 0, 1, 5 ),	CF( HYBRID, 7, 0, 3, 15 ), CF( AVX10, 7, 1, 3, 19 ),
		CF( MCDT_NO, 7, 2, 3, 5 )
		#undef CF
		/* for copying: value-list and x-macro-list
			cpu_feature::ACPI,	cpu_feature::MMX,	 cpu_feature::SSE,	 cpu_feature::SSE2,
			cpu_feature::AVX2,	cpu_feature::SSE3,	 cpu_feature::SSSE3, cpu_feature::FMA,
			cpu_feature::SSE41, cpu_feature::HYBRID, cpu_feature::SSE42, cpu_feature::AESNI,
			cpu_feature::AVX,	cpu_feature::F16C,	 cpu_feature::AVX10, cpu_feature::HTT,
			cpu_feature::TSC,	cpu_feature::TM,	 cpu_feature::TM2,	 cpu_feature::MCDT_NO

			X( ACPI )	X( MMX )	X( SSE )	X( SSE2 )	X( AVX2 )	X( SSE3 )	X( SSSE3 )
			X( FMA )	X( SSE41 )	X( HYBRID )	X( SSE42 )	X( AESNI )	X( AVX )	X( F16C )
			X( AVX10 )	X( HTT )	X( TSC )	X( TM )		X( TM2 )	X( MCDT_NO )
		 */
	};
	inline unsigned get_leaf( cpu_feature feat )	{ return ( ( ( unsigned ) feat ) >> 8 ) & 0xff; }
	inline unsigned get_subleaf( cpu_feature feat ) { return ( ( ( unsigned ) feat ) >> 16 ) & 0xff; }
	inline unsigned get_register( cpu_feature feat ) { return ( ( ( unsigned ) feat ) >> 5 ) & 0x3; }
	inline unsigned get_bit( cpu_feature feat ) { return ( ( unsigned ) feat ) & 0x1f; }
	// clang-format on

	/**
	 * 	This structure is made to describe one logical CPU in your system
	 * 	→ its APIC_ID
	 * 	→ a list of this id masked and shifted as different domain IDs
	 * 	→ the content of CPUID(1) — showing CPU caps (testable)
	 */
	struct cpu_id
	{
		static int	 fmt_width;	   // static - will be set according to maximum apic_id encountered.
		id_list		 masked_ids{}; // domain-masked apic_ids
		cpuid_result caps{};	   // result of Leaf 0x01
		apic_id		 id{ UINT_MAX }; // actual apic_id of this CPU
		// additional information from other leafs:
		uint32_t	 model_id{ UINT_MAX };
		uint8_t		 core_type{ 0xff };
		uint8_t		 max_leaf{ 0x3 };
		cpu_id() = default;
		cpu_id( apic_id in_id )
			: cpu_id( 3, in_id )
		{}
		cpu_id( unsigned leafs, apic_id in_id, cpuid_result in_leaf1 = {} )
			: id( in_id )
			, caps( in_leaf1 )
			, max_leaf( leafs )
		{ fmt_width = max( fmt_width, int( log2( in_id ) / 4 + 1 ) ); }
		operator bool() const noexcept { return !masked_ids.empty() && id != UINT_MAX; }
		operator string() const noexcept
		{
			string list, fmtstr = '{' + format( ":#0{:d}X", fmt_width + 2 ) + '}';
			for ( auto i( 0u ); i < masked_ids.size(); ++i )
			{
				if ( !list.empty() ) list.insert( list.begin(), ':' );
				list.insert( 0, vformat( fmtstr, make_format_args( masked_ids[ i ] ) ) );
			}
			return vformat( fmtstr + "({:s})", make_format_args( id, list ) );
		}
		bool operator()( cpu_feature feature ) const
		{
			const auto f = ( unsigned ) feature;
			if ( auto l = get_leaf( feature ); l == 1 ) // Intel-Features of CPUID.01h
				return ( caps.x.Registers[ f / 32 ] >> ( f % 32 ) ) & 1;
			else if ( l <= max_leaf )
				return call_cpuid( l, get_subleaf( feature ) )
						   .x.Registers[ get_register( feature ) ]
					   & ( 1 << get_bit( feature ) );
			else return false;
		}
	};

	/*
	 * The enumeration of domain identifiers and these need to each match
	 * the value as specified by CPUID.1F and CPUID.B documentation.
	 *
	 * Using an X-macro driven approach, here …
	 */
	// clang-format off
	#define CPU_DOMAINS( X )	X( InvalidDomain )						\
			X( LogicalDomain )	X( CoreDomain )		X( ModuleDomain )	\
			X( TileDomain )		X( DieDomain )		X( DieGrpDomain )
	#define X( name ) name,
	enum cpu_domain { CPU_DOMAINS( X ) };
	#undef X
	// clang-format on
	/*
	 *	The maximum number of enumerated domains, since X2APIC is 32 bits
	 *	there really can't be more than 32 domains enumerated.
	 */
	constexpr unsigned MAXIMUM_DOMAINS = 32;
	constexpr unsigned MAX_PROCESSORS  = 1024;

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
	struct apic_id_bit_layout
	{
		cpu_domain domain{ InvalidDomain };
		unsigned   shift{};
		mask_map   relative_masks;
	};
	class apicid_bit_layouts : public vector< apic_id_bit_layout >
	{
		static constexpr unsigned number_of_apic_bits = 32;

	  public:
		cpu_domain			top_domain{ InvalidDomain };
		unsigned			domains() const { return size() + 1; }

		// non-const access operator shall emplace/resize on demand!
		apic_id_bit_layout &operator[]( size_t index )
		{
			while ( index >= size() )
				emplace_back( ( cpu_domain ) ( back().domain + 1 ), 0u, mask_map{} );
			return vector< apic_id_bit_layout >::operator[]( index );
		}
		// likewise
		const apic_id_bit_layout operator[]( size_t index ) const
		{
			if ( index < size() ) return vector< apic_id_bit_layout >::operator[]( index );
			else return { InvalidDomain, 0, mask_map{} };
		}
	};

#pragma endregion
#pragma region topology class

	/*	cpu topology class:
	 *	- runs 'the topology acquisition' code in the CTor,
	 *	  thus a returned object can be queried right away.
	 */
	class cpu_topo
	{
#define X( name ) #name,
		static constexpr const char *lvl_base_names[] = { CPU_DOMAINS( X ) };
#undef X
		// current system's CPUID capabilities (1|B|1F)
		unsigned sourceLeaf{ 1 };

	  public:
		// list of actual cpu_id mappings, including pre-masked IDs
		vector< cpu_id >						 cpu_ids;
		// current system's topology level masks and their names, strongly ordered ascending
		map< unsigned, pair< id_mask, string > > level_masks_names;
		// a vector of maps to count masked apic_ids - describes how many
		// logical cores share the respective masked apic id
		vector< map< unsigned, int > >			 lvl_ids;
		apicid_bit_layouts						 abl;

		cpu_topo( bool force_legacy_detection = false );

		// count items of a specific domain (like logical cpu count, core count, tile, package)
		int		countLevel( cpu_domain lvl ) const noexcept;

		cpu_id	id( size_t index ) const noexcept;

		id_list optimalProcessAffinity( int thread_count, bool prefer_performance = true );

	  protected:
		void		 parse_cpuid_legacy( const cpuid_result &zero_zero );
		void		 parse_cpuid_modern();

		unsigned int create_topology_shift( unsigned int count );
		void		 build_up_apic_ids( cpuid_result &CpuidRegisters );

		void		 finish_topology();
	};
#pragma endregion
} // namespace cpu_info