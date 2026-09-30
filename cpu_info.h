#pragma once
/**
 * 	cpu_info:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
 *
 * 	Usage:
 * ========
 * 	namespace 'cpu_info' functions:
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
 *				→ class will enumerate all available CPUIDs ON EVERY SINGLE LOGICAL CPU
 *				→ query caps extended to all feature-bits found in the intel docs,
 *
 * 	Step #3✗:	also query "memory"-items, so scoring by shared / non-shared ids becomes possible.
 *
 *	Step #4✓:	Implement platform-independent 'cpu_set'.  It's mostly called the same on both
 *			 	 platforms of current interest, but implemented differently.
 */
#include <cpu_id.h>	   // include the code to acquire necessary data
#include <cpu_set.hpp> // platform-independent thread affinities …

namespace cpu_info
{
	/*	cpu topology class:
	 *	- runs 'the topology acquisition' code in the CTor,
	 *	  thus a returned object can be queried right away.
	 *	- also keeps the process affinity mask "around", so further created
	 *	  thread affinities can be masked by the process mask.
	 */
	class cpu_topo
	{
	  public:
		// system-assigned process affinity mask
		const cpu_set							 process_affinity;
		// list of actual cpu_id mappings, including pre-masked IDs
		vector< cpu_id >						 cpu_ids;
		// current system's topology level masks and their names, strongly ordered ascending
		map< unsigned, pair< id_mask, string > > level_masks_names;
		// a vector of maps to count masked apic_ids - describes how many
		// logical cores share the respective masked apic id
		vector< map< unsigned, int > >			 lvl_ids;

		cpu_topo(); // throws in case CPUID instruction is not available
					// or thread affinity cannot be set.

		size_t count() const noexcept { return cpu_ids.size(); }
		// count items of a specific domain (like logical cpu count, core count, tile, package)
		size_t countLevel( cpu_domain lvl ) const noexcept
		{
			if ( lvl == cpu_domain::InvalidDomain || lvl_ids.size() < ( size_t ) lvl ) return 1;
			return lvl_ids[ lvl - 1 ].size();
		}
		// throwing accessor, throws in case of invalid index
		const cpu_id &operator[]( size_t index ) const { return cpu_ids[ index ]; }

	  protected:
		void build_idlist();
		void parse_topology();
	};
} // namespace cpu_info
