#pragma once
/**
 * 	cpu_info:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class …
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
		const cpu_set				  process_affinity;
		// list of actual cpu_id mappings, including pre-masked IDs
		vector< cpu_id >			  cpu_ids;
		// a vector of maps to count masked apic_ids - describes how many
		// logical cores share the respective masked apic id
		vector< map< apic_id, int > > lvl_ids;

		cpu_topo(); // throws in case CPUID instruction is not available
					// or thread affinity cannot be set.
		void	   refresh() noexcept;

		// returns the number of logical cpus accessible to the callee (without further OS calls)
		size_t	   count() const noexcept { return cpu_ids.size(); }
		cpu_domain max_domain() const noexcept { return cpu_domain( lvl_ids.size() ); }
		// count items of a specific domain (like logical cpu count, core count, tile, package)
		size_t	   count_domain( cpu_domain lvl ) const noexcept
		{
			if ( lvl == cpu_domain::InvalidDomain || lvl_ids.size() < ( size_t ) lvl ) return 1;
			return lvl_ids[ lvl - 1 ].size();
		}
		// throwing accessor, throws in case of invalid index
		const cpu_id &operator[]( size_t index ) const noexcept { return cpu_ids[ index ]; }

		bool		  knows_efficiency() const noexcept
		{ return cpu_ids[ 0 ].max_leaf() >= 0x1a; /* core type available */ }

	  protected: // internal operations
		inline unsigned id_leaf( int index = 0 ) const { return cpu_ids[ index ].id_leaf(); }
		void			parse_topology();
	};
} // namespace cpu_info
