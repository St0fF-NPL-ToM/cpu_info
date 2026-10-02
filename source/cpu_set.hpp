#pragma once
/**
 * 	cpu_set:	trying to get Intel's official code from
 *
 * 				https://github.com/intel/SDM-Processor-Topology-Enumeration
 *
 * 				ported to a simple cpp class … does not work.
 *
 *  This is the header of the cpu_set subclass, implementing task affinity.
 *
 *	ATTN:	This implementation does not try to reinvent cpu_sets, as known
 *			from different OSes.  It is a simple try on capsuling OS issues
 *			from application code, giving a platform independent option of
 *			saying: "please let my thread run on these CPUs, only."
 */

#ifdef _WIN32
	#define NOMINMAX
	#include <Windows.h>
	#include <vector>
	#include <algorithm>
#elifdef linux
	#include <sched.h>
	#include <unistd.h>
	#include <sys/sysinfo.h>
	#include <utility>
#endif

namespace cpu_info
{
	class cpu_set final
	{
	  public:
		enum class init_type { thread, empty };
		// Query system CPU count, but only depending on the currently assigned group
		// (in case we're running on a very phat system …)
		/*  static unsigned get_logical_cpu_count() noexcept; */

		/*  cpu_set() noexcept;					   */ // query the current process' cpu_set
		/*  cpu_set( int logical_cpu ) noexcept;   */ // create a cpu_set with single affinity
		/*  cpu_set( const cpu_set& o ) = default; */ // create a copy
		/*  cpu_set( cpu_set&& o )		= default; */ // move from another instance
		/*  ~cpu_set() noexcept; */

		/*  	operator bool() const noexcept;    */
		bool	applyToCurrentThread() const noexcept { return apply(); }

		// produce a new set from operations
		cpu_set operator+( int cpu_id ) const noexcept { return operator|( { cpu_id } ); }
		cpu_set operator-( int cpu_id ) const noexcept { return operator&( { cpu_id } ); }
		/*  cpu_set	 operator|( const cpu_set& o ) const noexcept; */ // unite
		/*  cpu_set	 operator&( const cpu_set& o ) const noexcept; */ // intersect
		/*  cpu_set	 operator^( const cpu_set& o ) const noexcept; */ // xor - remove shared

		// modify inline
		/*  cpu_set& operator+=( int cpu_id ) noexcept; */
		/*  cpu_set& operator-=( int cpu_id ) noexcept; */
		/*  cpu_set& operator|=( const cpu_set& o ) noexcept;   */
		/*  cpu_set& operator&=( const cpu_set& o ) noexcept;   */
		/*  cpu_set& operator^=( const cpu_set& o ) noexcept;   */

	  protected:
		/* internal implementations following */
#ifdef linux
		int		   sz{ 0 };
		cpu_set_t* set{ nullptr };

		bool	   query() noexcept
		{
			if ( sched_getaffinity( getpid(), sz, set ) ) return false;
			return true;
		}

		bool apply() const noexcept
		{
			if ( !set ) return false;
			else if ( sched_setaffinity( getpid(), sz, set ) ) return false;
			else return true;
		}
		// Now, all above-mentioned functions get declared for linux:
	  public:
		static inline unsigned get_logical_cpu_count() noexcept { return get_nprocs(); }

		cpu_set( init_type _init = init_type::thread ) noexcept
			: sz( get_nprocs() )
			, set( CPU_ALLOC( sz ) )
		{
			CPU_ZERO_S( sz, set );
			if ( _init == init_type::thread ) query();
		}

		cpu_set( int logical_cpu ) noexcept
			: cpu_set( init_type::empty )
		{ CPU_SET_S( logical_cpu, sz, set ); }

		~cpu_set() noexcept
		{
			if ( set )
			{
				cpu_set_t* old{ nullptr };
				std::swap( set, old );
				CPU_FREE( old );
			}
		}
		inline	operator bool() const noexcept { return set != nullptr; }

		cpu_set operator&( const cpu_set& o ) const noexcept
		{
			cpu_set result;
			CPU_AND_S( sz, result.set, set, o.set );
			return result;
		}
		cpu_set operator|( const cpu_set& o ) const noexcept
		{
			cpu_set result;
			CPU_OR_S( sz, result.set, set, o.set );
			return result;
		}
		cpu_set& operator+=( int cpu_id ) noexcept
		{
			CPU_SET_S( cpu_id, sz, set );
			return *this;
		}
		cpu_set& operator-=( int cpu_id ) noexcept
		{
			CPU_CLR_S( cpu_id, sz, set );
			return *this;
		}
		cpu_set& operator|=( const cpu_set& o ) noexcept
		{
			CPU_OR_S( sz, set, set, o.set );
			return *this;
		}
		cpu_set& operator&=( const cpu_set& o ) noexcept
		{
			CPU_AND_S( sz, set, set, o.set );
			return *this;
		}
		cpu_set& operator^=( const cpu_set& o ) noexcept
		{
			CPU_XOR_S( sz, set, set, o.set );
			return *this;
		}
#elifdef _WIN32
	  protected:
		GROUP_AFFINITY ga{ .Group = ALL_PROCESSOR_GROUPS };
		DWORD_PTR	   sysAffinity;
		cpu_set( WORD group, KAFFINITY mask, DWORD_PTR sa )
			: ga( mask, group, {} )
			, sysAffinity( sa )
		{}
		bool query() noexcept
		{
			/*	Multiple ways lead to Rome … we need: a cpu-group id AND an affinity mask
			 *	→ easiest solution:
			 *		- GetCurrentProcessorNumberEx:	currently active cpu-group id
			 *		- GetProcessAffinityMask:		process affinity and system affinity
			 *		(logically, these affinities cover the previously acquired group id)
			 */
			HANDLE			 prc{ GetCurrentProcess() };
			PROCESSOR_NUMBER pn{};
			GetCurrentProcessorNumberEx( &pn );
			ga.Group = pn.Group;
			return GetProcessAffinityMask( prc, &ga.Mask, &sysAffinity );
		}
		bool apply() const noexcept
		{
			if ( ga.Group == ALL_PROCESSOR_GROUPS ) return false;
			else return SetThreadGroupAffinity( GetCurrentThread(), &ga, nullptr );
		}

	  public:
		static inline unsigned get_logical_cpu_count() noexcept
		{
			PROCESSOR_NUMBER pn{};
			GetCurrentProcessorNumberEx( &pn );
			return GetActiveProcessorCount( pn.Group );
		}
		cpu_set( init_type _init = init_type::thread ) noexcept
			: ga( {} )
		{
			query();
			if ( _init == init_type::empty ) ga.Mask = KAFFINITY{};
		}
		cpu_set( int logical_cpu ) noexcept
			: cpu_set()
		{
			ga.Mask = // create single cpu mask
				( KAFFINITY ) ( 1 << std::min( ( DWORD ) logical_cpu,
											   GetActiveProcessorCount( ga.Group ) - 1 ) );
		}
				operator bool() const noexcept { return ga.Group != ALL_PROCESSOR_GROUPS; }

		cpu_set operator|( const cpu_set& o ) const noexcept
		{ return cpu_set( ga.Group, ( ga.Mask | o.ga.Mask ) & sysAffinity, sysAffinity ); }

		cpu_set operator&( const cpu_set& o ) const noexcept
		{ return cpu_set( ga.Group, ( ga.Mask & o.ga.Mask ) & sysAffinity, sysAffinity ); }

		cpu_set operator^( const cpu_set& o ) const noexcept
		{ return cpu_set( ga.Group, ( ga.Mask ^ o.ga.Mask ) & sysAffinity, sysAffinity ); }

		cpu_set& operator+=( int cpu_id ) noexcept
		{
			ga.Mask |= ( 1 << cpu_id ) & sysAffinity;
			return *this;
		}
		cpu_set& operator-=( int cpu_id ) noexcept
		{
			ga.Mask &= ~( ( 1 << cpu_id ) & sysAffinity );
			return *this;
		}
		cpu_set& operator|=( const cpu_set& o ) noexcept
		{
			if ( ga.Group == o.ga.Group ) ga.Mask |= o.ga.Mask & sysAffinity;
			return *this;
		}
		cpu_set& operator&=( const cpu_set& o ) noexcept
		{
			if ( ga.Group == o.ga.Group ) ga.Mask &= o.ga.Mask & sysAffinity;
			return *this;
		}
		cpu_set& operator^=( const cpu_set& o ) noexcept
		{
			if ( ga.Group == o.ga.Group ) ga.Mask ^= o.ga.Mask & sysAffinity;
			return *this;
		}

#endif
	};
} // namespace cpu_info
