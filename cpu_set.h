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
#elifdef linux
	#include <sched.h>
	#include <unistd.h>
	#include <sys/sysinfo.h>
#endif

namespace cpu_info
{
	class cpu_set final
	{
	  public:
		// Query system CPU count, but only depending on the currently assigned group
		// (in case we're running on a very phat system …)
		static unsigned get_logical_cpu_count() noexcept;

		cpu_set() noexcept;					   // query the current process' cpu_set
		cpu_set( int logical_cpu ) noexcept;   // create a cpu_set with single affinity
		cpu_set( const cpu_set& o ) = default; // create a copy
		cpu_set( cpu_set&& o )		= default; // move from another instance
		~cpu_set() noexcept;

				 operator bool() const noexcept;
		bool	 applyToCurrentThread() const noexcept { return apply(); }

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
		bool query() noexcept;
		bool apply() const noexcept;
#ifdef linux
		int		   sz{ 0 };
		cpu_set_t* set{ nullptr };
#elifdef _WIN32
		GROUP_AFFINITY ga{ .Group = ALL_PROCESSOR_GROUPS };
#endif
	};
} // namespace cpu_info
