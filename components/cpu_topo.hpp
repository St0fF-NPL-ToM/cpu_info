#pragma once
/**
 *
 */
#include <cpu_id.hpp>

namespace cpu_info
{

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
		{ return cpu_ids[ 0 ].efficiency() != cpu_efficiency::unknownEff; }

	  protected: // internal operations
		void parse_topology();
	};

} // namespace cpu_info
