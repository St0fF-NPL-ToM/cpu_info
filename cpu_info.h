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
 * 	Step #1✓:	simple cpu enumeration on a system with subclassing by CPU domains
 * 				- query count of CPUs of a domain
 * 				- retrieve (un)masked APIC IDs per cpu
 * 	→ 	Solves the question of "how many threads do make sense in certain scenarios"
 *		by comparing different level counts.
 *	→	[x] feature-complete!
 *
 * 	Step #2✓:	query (if available) core_types (efficiency/performance etc.)
 *
 * 	Step #3✗:	also query "memory"-items, so scoring by shared / non-shared ids becomes possible.
 * */

#include <cpu_id.h>

namespace cpu_info
{
	/* static interface functions:
	 */
	extern cpuid_result call_cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;
	extern unsigned		get_logical_cpu_count() noexcept;

	/*
	 *	bind to cpu(s), return previous affinity list
	 *	→ on failure, an empty "previous" list is returned! So check the result!
	 */
	extern id_list		setThreadAffinity( const id_list &target_ids );
	inline id_list		bind_thread_to_cpu( unsigned ProcessorNumber )
	{ return setThreadAffinity( { ProcessorNumber } ); }

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

		cpu_topo(); // throws in case CPUID instruction is not available
					// or thread affinity cannot be set.

		// count items of a specific domain (like logical cpu count, core count, tile, package)
		int			  countLevel( cpu_domain lvl ) const noexcept;

		// throws in case of invalid index
		const cpu_id &id( size_t index ) const;

		id_list		  optimalProcessAffinity( int thread_count, bool prefer_performance = true );

	  protected:
		void		 build_idlist();
		void		 parse_topology();

		unsigned int create_topology_shift( unsigned int count );

		void		 finish_topology();
	};
} // namespace cpu_info