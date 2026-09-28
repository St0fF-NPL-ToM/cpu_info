/**
 * 	cpu_set:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_set subclass, implementing task affinity.
 */
#include <cpu_set.h>

#ifdef linux
	#include <utility>

namespace cpu_info
{
	/* static */
	unsigned cpu_set::get_logical_cpu_count() noexcept
	{ return get_nprocs(); }

	cpu_set::cpu_set() noexcept
		: sz( get_nprocs() )
		, set( CPU_ALLOC( sz ) )
	{
		CPU_ZERO_S( sz, set );
		query();
	}

	cpu_set::cpu_set( int logical_cpu ) noexcept
		: sz( get_nprocs() )
		, set( CPU_ALLOC( sz ) )
	{
		CPU_ZERO_S( sz, set );
		CPU_SET_S( logical_cpu, sz, set );
	}

	cpu_set::~cpu_set() noexcept
	{
		if ( set )
		{
			cpu_set_t* old{ nullptr };
			std::swap( set, old );
			CPU_FREE( old );
		}
	}

	cpu_set::operator bool() const noexcept
	{ return set != nullptr; }

	bool cpu_set::query() noexcept
	{
		if ( sched_getaffinity( getpid(), sz, set ) ) return false;
		return true;
	}

	bool cpu_set::apply() const noexcept
	{
		if ( !set ) return false;
		else if ( sched_setaffinity( getpid(), sz, set ) ) return false;
		else return true;
	}
} // namespace cpu_info

#elifdef _WIN32

namespace cpu_info
{
	/* static */
	unsigned cpu_set::get_logical_cpu_count() noexcept
	{
		PROCESSOR_NUMBER pn{};
		GetCurrentProcessorNumberEx( &pn );
		return GetActiveProcessorCount( pn.Group );
	}

	cpu_set::cpu_set() noexcept
		: ga( {} )
	{ query(); }

	cpu_set::cpu_set( int logical_cpu ) noexcept
		: ga( {} )
	{
		PROCESSOR_NUMBER pn{};
		GetCurrentProcessorNumberEx( &pn );
		ga.Group = pn.Group;
		ga.Mask	 = // create single cpu mask
			( KAFFINITY ) ( 1 << std::max( logical_cpu, GetActiveProcessorCount( pn.Group ) ) );
	}

	cpu_set::~cpu_set() noexcept
	{ ga.Group = ALL_PROCESSOR_GROUPS; }

	cpu_set::operator bool() const noexcept
	{ return ga.Group != ALL_PROCESSOR_GROUPS; }

	bool cpu_set::query() noexcept
	{
		// retrieve cpu-group, because as an end-user-application there is no need
		// to use more than one cpu-group.
		if ( GetProcessGroupAffinity( GetCurrentThread(), &ga ) ) return true;
		else return false;
	}

	bool cpu_set::apply() const noexcept
	{
		if ( ga.Group == ALL_PROCESSOR_GROUPS ) return false;
		else return SetThreadGroupAffinity( GetCurrentThread(), &ga, nullptr );
	}
} // namespace cpu_info
#endif
