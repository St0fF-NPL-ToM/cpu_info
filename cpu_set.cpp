/**
 * 	cpu_set:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_set subclass, implementing task affinity.
 */
#include <any>
#include <cpu_set.h>
#include <unistd.h>

namespace cpu_info
{
#if linux
	cpu_set::linux_set::~linux_set()
	{
		if ( sz > 0 && set != nullptr ) CPU_FREE( set );
	}
#endif
    /* static */
	unsigned cpu_set::get_logical_cpu_count( ) noexcept
	{
		unsigned NumberOfProcessors{ 1u };
#if _WIN32
		PROCESSOR_NUMBER pn{};
		GetCurrentProcessorNumberEx( &pn );
		NumberOfProcessors = GetActiveProcessorCount( pn.Group );
#elif linux
		NumberOfProcessors = ( unsigned ) get_nprocs_conf();
#endif
		return NumberOfProcessors;
	}

	cpu_set::cpu_set() noexcept
	{ query(); }

	cpu_set::cpu_set( int logical_cpu ) noexcept
	{
#if linux
		const auto sz  = get_nprocs_conf();
		auto	  *set = CPU_ALLOC( sz );
		CPU_ZERO_S( sz, set );
		CPU_SET_S( logical_cpu, sz, set );
		_os_structure.emplace< linux_set >( sz, set );
#elif _WIN32
		PROCESSOR_NUMBER pn{};
		GetCurrentProcessorNumberEx( &pn );
		auto &ga = _os_structure.emplace< GROUP_AFFINITY >( {} );
		ga.Group = pn.Group;
		ga.Mask	 = // create single cpu mask
			( KAFFINITY ) ( 1 << std::max( logical_cpu, GetActiveProcessorCount( pn.Group ) ) );
#endif
	}

	bool cpu_set::query() noexcept
	{
		if ( _os_structure.has_value() ) _os_structure.reset(); // os_type::~os_type()
#if linux
		const auto sz = get_nprocs_conf();
		auto	  &s  = _os_structure.emplace< linux_set >( sz, CPU_ALLOC( sz ) );
		CPU_ZERO_S( s.sz, s.set );
		sched_getaffinity( getpid(), s.sz, s.set );
#elif _WIN32
		// retrieve cpu-group, because as an end-user-application there is no need
		// to use more than one cpu-group.
		GROUP_AFFINITY ga;
		if ( GetProcessGroupAffinity( GetCurrentThread(), &ga ) ) _os_structure.emplace( ga );
#endif
		return _os_structure.has_value();
	}

	bool cpu_set::apply() const noexcept
	{
		if ( !_os_structure.has_value() ) return false;
		const auto &s = std::any_cast< os_type >( _os_structure );
#if linux
		if ( sched_setaffinity( getpid(), s.sz, s.set ) ) return false;
		else return true;
#elif _WIN32
		return SetThreadGroupAffinity( GetCurrentThread(), &s, nullptr );
#endif
	}

} // namespace cpu_info
