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
 * 		4Regs 		call_cpuid( Leaf, Sub ) 		→ execute the respective CPUID
 *		void 		bind_thread_to_cpu( cpuNumber ) → what it's called …
 *		unsigned 	get_logical_cpu_count() 		→ again, the naming speaks …
 *
 *	class cpu_topo:	simply instantiate, then query.
 *
 * 	→
 */

#include <vector>
#include <map>
#include <string>

namespace cpu_info
{
#pragma region declare uses, defines, and structures

	using namespace std;

	using id_list = vector< unsigned >;
	using id_mask = unsigned;

	struct cpu_id : pair< unsigned, id_list >
	{
		operator bool() const noexcept { return !second.empty(); }
	};

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

	/*
	 * The enumeration of domain identifiers and these need to each match
	 * the value as specified by CPUID.1F and CPUID.B documentation.
	 *
	 * Using an X-macro driven approach, here …
	 */
#define CPU_DOMAINS( X )                                                                           \
	X( InvalidDomain )                                                                             \
	X( LogicalDomain )                                                                             \
	X( CoreDomain )                                                                                \
	X( ModuleDomain )                                                                              \
	X( TileDomain )                                                                                \
	X( DieDomain )                                                                                 \
	X( DieGrpDomain )
#define X( name ) name,
	enum cpu_domain { CPU_DOMAINS( X ) };
#undef X

	/* The maximum number of enumerated domains, since X2APIC is 32 bits
	 * there really can't be more than 32 domains enumerated.
	 */
	constexpr unsigned MAXIMUM_DOMAINS = 32;
	constexpr unsigned MAX_PROCESSORS  = 1024;
	struct APICID_BIT_LAYOUT_CTX
	{
		/* To support this as legacy APIC, the structure will contain the number
		 * of bits that represent an APIC ID, which has been 4, 8 and 32(Today).
		 *
		 * This code only will set it to 8 or 32.
		 */
		unsigned int NumberOfApicIdBits;
		/*
		 * These are a cache of CPUID Topology as returned from CPUID.1F or CPUID.B.
		 *
		 * The usage beyond mirroring the values in a simple structure is that these
		 * values can contain a collapsed version from Unknown Domains to a list of
		 * all known domains or other number of levels.
		 */
		unsigned int ShiftValues[ MAXIMUM_DOMAINS ];
		unsigned int ShiftValueDomain[ MAXIMUM_DOMAINS ];
		/*
		 * This is a domain relative where the index is based on the domain
		 * level index.  The second index determines the relative to the current
		 * domain mask.  The index where both entries are the current domain represents
		 * a global mask to ID this domain level globally.
		 *
		 * The indexes then move to the next higher domain creating a relative mask from the
		 * current domain relative to the second domain level index.
		 *
		 */
		unsigned int DomainRelativeMasks[ MAXIMUM_DOMAINS ][ MAXIMUM_DOMAINS ];
		/*
		 * The top index in the above matrix that contains the package domain.
		 */
		unsigned int PackageDomainIndex;
		/*
		 * This is a string that allows a description to be passed from the parsing function
		 * to the general display function for context.
		 */
		char		 szDescription[ 256 ];
	};

	class mask_map : public map< unsigned, unsigned >
	{
	  public:
		using BASE = map< unsigned, unsigned >;
		using BASE::map;
		const unsigned operator[]( unsigned key ) const
		{
			if ( contains( key ) ) return at( key );
			else return 0;
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
		static constexpr unsigned		number_of_apic_bits = 32;
		static const apic_id_bit_layout empty_layout;

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
		const apic_id_bit_layout &operator[]( size_t index ) const
		{
			if ( index < size() ) return vector< apic_id_bit_layout >::operator[]( index );
			else return empty_layout;
		}
	};

#pragma endregion

	/* static interface functions
	 */
	extern cpuid_result call_cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;
	extern void			bind_thread_to_cpu( unsigned ProcessorNumber );
	extern unsigned		get_logical_cpu_count() noexcept;

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
		// current system's logical core apic ids
		id_list									 apic_cpu_ids;
		// current system's topology level masks and their names, strongly ordered ascending
		map< unsigned, pair< id_mask, string > > level_masks_names;
		// list of actual cpu_id mappings: apic_id and list of masked sub-ids
		vector< cpu_id >						 cpu_ids;
		// a vector of maps to count masked apic_ids - describes how many
		// logical cores share the respective masked apic id
		vector< map< unsigned, int > >			 lvl_ids;
		apicid_bit_layouts						 abl;

		cpu_topo( bool force_legacy_detection = false );

		// count items of a specific domain (like logical cpu count, core count, tile, package)
		int	   countLevel( cpu_domain lvl ) const noexcept;

		cpu_id id_of( size_t index ) const noexcept;
		string id_string( size_t index ) const noexcept;

	  protected:
		void		 parse_cpuid_legacy( const cpuid_result &zero_zero );
		void		 parse_cpuid_modern();

		void		 create_domain_masks( APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx );
		void		 create_domain_masks();
		unsigned int CreateTopologyShift( unsigned int count );
		void		 PopulatePlatformApicIds( cpuid_result &CpuidRegisters );

		void ThreeDomainFinalize( unsigned int PackageShift, unsigned int LogicalProcessorShift );
		void ManyDomainFinalize( APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx );
		void finish_topology();

		pair< string, string > getFmt() const noexcept;
	};

	/*
	 *	Extra functions that may be helpful on larger systems
	 *	when using excessive threading in your application.
	 */
	id_list optimalProcessAffinity( int thread_count, bool prefer_performance = true,
									cpu_topo topology = {} );
	void	setThreadAffinity( const id_list &target_ids );
} // namespace cpu_info