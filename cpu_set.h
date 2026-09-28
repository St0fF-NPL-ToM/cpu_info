#pragma once
/**
 * 	cpu_set:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_set subclass, implementing task affinity.
 */

#ifdef _WIN32
	#define NOMINMAX
	#include <Windows.h>
#endif

#include <any>

#ifdef linux
	#include <sched.h>
	#include <unistd.h>
	#include <sys/sysinfo.h>
#endif

namespace cpu_info
{
	class cpu_set
	{
	  public:
        // Query system CPU count, but only depending on the currently assigned group
        // (in case we're running on a very phat system …)
        static unsigned get_logical_cpu_count() noexcept;

		cpu_set() noexcept;					   // query the current process' cpu_set
		cpu_set( int logical_cpu ) noexcept;   // create a cpu_set with single affinity
		cpu_set( const cpu_set& o ) = default; // create a copy
		cpu_set( cpu_set&& o )		= default; // move from another instance

				 operator bool() const noexcept { return _os_structure.has_value(); }
		bool	 applyToCurrentThread() const noexcept { return apply();}

		// produce a new set from operations
		cpu_set	 operator+( int cpu_id ) const noexcept;
		cpu_set	 operator-( int cpu_id ) const noexcept;
		cpu_set	 operator&( const cpu_set& o ) const noexcept;
		cpu_set	 operator|( const cpu_set& o ) const noexcept;

		// modify inline
		cpu_set& operator+( int cpu_id ) noexcept;
		cpu_set& operator-( int cpu_id ) noexcept;
		cpu_set& operator&( const cpu_set& o ) noexcept;
		cpu_set& operator|( const cpu_set& o ) noexcept;

	  protected:
		std::any _os_structure;
#if linux
		struct linux_set
		{
			int		   sz;
			cpu_set_t* set;
			~linux_set();
		};
		using os_type = linux_set;
#elif _WIN32
		using os_type = GROUP_AFFINITY;
#endif
		bool query() noexcept;
		bool apply() const noexcept;
	};
} // namespace cpu_info
