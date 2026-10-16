/**
 *	cpu_info - thread pool test example
 *	===================================
 *	This example basically shows how to use the cpu_info namespace's "pool".
 *
 */
#define CPU_INFO_POOL_LOG
#include <cpu_pool.hpp>
#include <iostream>
#include <string>
#include <functional>
#include <utility>
#include <format>
#include <iosfwd>

// To keep it simple, let's "use" both namespaces
using namespace std;
using namespace cpu_info;

mutex mu;

// make sure every output attempt gets a safe slot
template < typename... ARGS >
void outputLine( ARGS&&... args )
{
	unique_lock l( mu );
	( cout << ... << args ) << endl;
}

// I never understood, why stl doesn't provide at least a partial template instantiation like this …
template < typename T >
ostream& operator<<( ostream& s, const vector< T >& v )
{
	auto it = v.begin();
	while ( it != v.end() ) s << *it << ( ++it == v.end() ? "" : ", " );
	return s;
}

void task_1s()
{
	const auto id = this_thread::get_id();
	outputLine( "starting task_1s: ", id );
	this_thread::sleep_for( chrono::seconds( 1 ) );
	outputLine( "finished task_1s: ", id );
}

uint64_t fak( int n )
{
	uint64_t   r{ 0 };
	const auto id = this_thread::get_id();
	outputLine( "starting fak(", n, "): ", id );
	for ( auto i: std::views::iota( 1, n ) )
		if ( r ) r *= ( uint64_t ) i;
		else r = 1ull;
	outputLine( "finished fak(", n, "): ", id, ", result: ", r );
	return r;
}

template < typename... ARGS >
void newState( pool& threads, ARGS&&... txt )
{
	auto st = threads.status();
	outputLine( txt..., " [", st.first, '|', st.second, "]" );
}
int main( int argc, const char* argv[] )
{
	{
		pool	   threads;
		const auto id = std::hash< std::string_view >()( "Wurstbrot" );
		newState( threads, "first state after create: " );

		auto zuk = threads.run_task( fak, 123 );
		newState( threads, "state after running fak: " );

		for ( auto i: views::iota( 0, 16 ) )
			threads.add_to_group( id, ( packaged_task< void() > ) bind( task_1s ) );
		newState( threads, "state after adding group: " );

		threads.run_group( id );
		newState( threads, "group started" );
		outputLine( "fak result: ", zuk.get() );

		zuk = threads.run_task( fak, 1337 );
		newState( threads, "longer fak: " );

		newState( threads, "wait for long fak" );
		outputLine( "fak result: ", zuk.get() );
		newState( threads, "wait for group" );
		newState( threads, "waiting returned: ", threads.await_group( id ) );
	}
	outputLine( "I'm done ..." );
	return 0;
}
