#pragma once
/**
 * 	cpu_info:	cpu_pool
 *  ====================
 *
 *  This is a versatile c++23 thread pool using cpu_topo for minimal "OS-scheduling-hinting" …
 *
 *  In case cpu_info detects the ability to distinguish efficient from performant cores, this
 *  pool will make as much use of it, as still seems to make sense.
 *
 *  Why this "restrictionism"?  Well, Intel® CPU specifications state that manually binding a
 *  thread of execution to a specific core may not be helpful at all, as it affects OS schedu-
 *  ling in a way, that it may not take the best decisions any more.
 *  Furthermore, cores that report efficiency, also report specific statistics the OS uses for
 *  scheduling.  That way a specific thread will most likely run once on a performance core
 *  and those statistics will make up for if the OS will reschedule further runs on performant,
 *  or on efficient cores.
 *
 *  Understanding this is crucial: The OS ALREADY KNOWS BEST (after a run or two of a specific
 *  thread).
 *
 *  There are only very few cases when "we know better". These cases are:
 *  - long-running math computations, sporadically executed
 *  - often but short-running math computations
 *
 *  Take off that last point: often and short-running heavy computes - let it sink in - yeah.
 *  This would be such a perfect fit for "the OS knows best after 1-2 runs".
 *
 *  So finally, there is only one "sporadically happening" option left … isn't it?
 *  … well, thinking a litle deeper:
 *  - long running
 *  - heavy computation
 *
 *  Both points also make it most probable, that the OS knows best after 1-2 context switches.
 *
 *  All-in-all, this means we really neither can save enery, nor save time:
 *  - in case "we know better", manually rebinding thread affinity also takes OS time, as well
 *    as non-uniform thread affinities also probably need more time for OS scheduling.
 *  Thus - we cannot say for sure - but we can say: to much effort for too little result!
 */
#include <map>
#include <vector>
#include <queue>
#include <future>
#include <thread>
#include <functional>

namespace cpu_info
{

	using task_group	= std::pair< std::atomic_int, std::vector< std::packaged_task< void() > > >;
	using task_group_id = std::size_t;
	using group_map		= std::map< task_group_id, task_group >;

	class pool
	{
		// private internal data
		group_map										groups;
		std::stop_source								stop;
		mutable std::recursive_mutex					mutex;
		std::condition_variable							notifier;
		std::queue< std::move_only_function< void() > > queue;
		std::vector< std::jthread >						p00l;
		int												free{}, active{};

		// internal operation: class thread may wait for work. It's private
		// and so it has private parent access.
		bool wait_for_work( std::unique_lock< std::mutex >& lock ) noexcept
		{
			while ( queue.empty() && !stop.stop_requested() )
			{
				--active, ++free;
				notifier.wait( lock );
				--free, ++active;
			}
			return !stop.stop_requested();
		}
		class thread
		{
			cpu_info::pool* pool;

		  public:
			thread( cpu_info::pool* _p )
				: pool( _p )
			{}
			void operator()()
			{
				std::unique_lock< std::mutex > lock( pool->mutex );
				while ( pool->wait_for_work( lock ) ) // run until killed
				{
					auto func = std::move( pool->queue.front() );
					pool->queue.pop();
					lock.unlock();
					if ( !pool->queue.empty() ) pool->notifier.notify_one();
					func(); // this will take time
					lock.lock();
				}
			}
		};

	  public:
		pool()
		{
			// threads will start, thus we need to protect internal data during CTor
			std::scoped_lock lock( mutex );
			// find out logical cpu count-1
			const size_t	 N = std::jthread::hardware_concurrency() - 1;
			for ( auto i: std::views::iota( 1ul, N ) )
				++active, p00l.emplace_back( thread( this ) );
			// As all threads try to acquire the mutex via their unique_lock right from the
			// start, it all begins right after the CTor ends, releasing the mutex.

			// Threads will - one after the other in no particular order - make a call to acquire a
			// task - of which none would be ready, yet - and goes to sleep until woken.  This will
			// happen to all threads, except if there were so many, that the main thread already
			// provided work.
		}
		/*
		 *  now for the advanced stuff: run_task - 2 variants:
		 *  1. set_and_forget_kind
		 *  2. returning a future
		 */
		void run_task( std::move_only_function< void() >&& task )
		{
			std::unique_lock< std::recursive_mutex > lock( mutex );
			queue.push( std::move( task ) );
			notifier.notify_one();
		}
		template < typename F, typename... Args >
		auto run_task( F&& f, Args&&... args ) -> std::future< decltype( f( args... ) ) >
		{
			std::packaged_task< decltype( f( args... ) )() > task(
				std::bind( std::forward< F >( f ), std::forward< Args >( args )... ) );
			auto future = task.get_future();
			run_task( ( std::move_only_function< void() >&& ) [task = std::move( task )]() mutable {
				std::move( task )();
			} );
			return future;
		}
		void run_group( task_group_id gid ) noexcept
		{
			if ( groups.contains( gid ) )
			{
				std::unique_lock< std::recursive_mutex > lock( mutex );
				auto&									 g{ groups.at( gid ) };
				const auto								 sz{ g.second.size() };
				g.first.fetch_add( sz );
				for ( auto tid: std::views::iota( 0ul, sz - 1 ) )
					run_task(
						[ this ]( task_group_id group_id, auto task_id )
						{
							std::invoke( groups.at( group_id ).second[ task_id ] );
							groups.at( group_id ).first.fetch_sub( 1, std::memory_order_release );
						},
						gid, tid );
			}
		}
		void run_in_group( task_group_id gid, std::packaged_task< void() >&& task ) noexcept
		{
			if ( groups.contains( gid ) )
			{
				groups.at( gid ).first.fetch_add( 1 );
				run_task(
					[ this, t = std::move( task ) ]( int g ) mutable
					{
						std::move( t )();
						groups.at( g ).first.fetch_sub( 1, std::memory_order_release );
					},
					gid );
			}
		}
		int add_to_group( task_group_id gid, std::packaged_task< void() >&& task ) noexcept
		{
			groups[ gid ].second.push_back( std::move( task ) );
			return groups[ gid ].second.size();
		}
		bool await_group( task_group_id gid ) const noexcept
		{
			if ( groups.contains( gid ) )
			{
				auto& a	 = groups.at( gid ).first;
				auto  cv = a.load( std::memory_order_relaxed );
				while ( cv > 0 && !stop.stop_requested() )
					a.wait( cv ), cv = a.load( std::memory_order_relaxed );
				return !stop.stop_requested();
			} else return false;
		}
	};
} // namespace cpu_info
