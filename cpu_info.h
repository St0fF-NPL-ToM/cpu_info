#pragma once
/**
 * 	cpu_topo:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
 */

#include <vector>
#include <map>
#include <string>

namespace cpu_info
{
	using namespace std;

	using id_list = vector< unsigned >;
	struct cpu_id : pair< unsigned, id_list >
	{
		operator bool() const noexcept { return !second.empty(); }
	};

#define MAX_PROCESSORS	1024
	/*
	 * The maximum number of enumerated domains, since X2APIC is 32 bits there
	 * really can't be more than 32 domains enumerated.
	 */
#define MAXIMUM_DOMAINS 32

	struct CPUID_REGISTERS
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
		 * The indexes then move to the next higher domian creating a relative mask from the
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
	/*
	 * The enumeration of domain identifiers and these need to each match
	 * the value as specified by CPUID.1F and CPUID.B documentation.
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

	/*
	 *	cpu topology class:
	 *	- runs 'the topology acquisition' code in the CTor,
	 *	  thus a returned object can be queried right away.
	 */
	class cpu_topo
	{
#define X( name ) #name,
		static constexpr const char *lvl_base_names[] = { CPU_DOMAINS( X ) };
#undef X

		unsigned					  sourceLeaf;
		vector< pair< int, string > > masks_names;

	  public:
		vector< cpu_id >			   cpu_ids;
		vector< map< unsigned, int > > lvl_ids;
		cpu_topo();

		int				countLevel( cpu_domain lvl ) const noexcept;

		CPUID_REGISTERS ReadCpuid( unsigned int Leaf, unsigned int Subleaf );
		cpu_id			id_of( size_t index ) const noexcept;
		string			id_string( size_t index ) const noexcept;

	  protected:
		void		 ParseCpu_CpuidLegacy( void );
		void		 ParseCpu_CpuidThreeDomain();
		void		 ParseCpu_CpuidManyDomain();
		void		 ParseCpu_CreateDomainMaskMatrix( APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx );

		unsigned int Tools_CreateTopologyShift( unsigned int count );
		id_list		 Tools_GatherPlatformApicIds();
		void		 Tools_SetAffinity( unsigned int ProcessorNumber );
		unsigned int Tools_GetNumberOfProcessors( void ) const noexcept;
		void ThreeDomainFinalize( unsigned int PackageShift, unsigned int LogicalProcessorShift );
		void ManyDomainFinalize( APICID_BIT_LAYOUT_CTX *pApicidBitLayoutCtx );

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