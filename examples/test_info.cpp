/**
 *	cpu_info - simple example using only cpu_info component
 *
 *	Please see how to instantiate the accessor cpu_info::CPUID.
 *
 *	This workaround was an idea to provide 2 different methods of querying, so with additional
 *	components, those can "hold on to results" and not call CPUID explicitly.
 *
 *	On top of that, this method resolves to generating as little code, as needed. If you use
 *	only the base component 'cpu_info', code will only be generated upon YOUR CALL.
 *
 *	Extensibility: cpu_id as a value-holder would only produce code using itself.
 */
#include <iostream>
#include <format>
#include <set>
#include <cpu_info.hpp>

using namespace cpu_info;

#define X( name, val ) #name,
constexpr const char *effistr[] = { EFFICIENCY_TYPE( X ) };
#undef X

int main( int argc, char *argv[] )
{
	// optimize your own code using one single Accessor: (makes sense for debug builds, should get
	// automatically optimized away in a release build)
	CPUID accessor; // please note: in the following, all calls are OVERspecified.
	std::cout
		<< brand_string< CPUID >( accessor ) << std::endl
		<< std::format(
			   "- Family: {:#04x}, Model: {:#04x}, Stepping: {:#04x}\n- Logical cores : {:d}\n",
			   family< CPUID >( accessor ), model< CPUID >( accessor ),
			   stepping< CPUID >( accessor ), cpu_info::count() );
	affinity			effi, perf;
	int					cpu{};
	std::set< APIC_id > coreIds;
	affinity_iterator	myAffinity{};
	do // What do we want to know about each single CPU?
	{
		// distinguish physical from logical cores
		coreIds.insert( apic_id< CPUID >( cpu_domain::CoreDomain, accessor ) );
		// build masks for efficient and performant cores
		const auto eff = efficiency< CPUID >( accessor );
		switch ( eff )
		{
			case cpu_efficiency::effficient: effi += cpu; break;
			case cpu_efficiency::performant: perf += cpu; break;
			default: break;
		}
		std::cout << std::format( "- Core #{:3d}: APIC_ID = {:#04x}, Efficiency: {:s}", cpu,
								  apic_id< CPUID >( cpu_domain::LogicalDomain, accessor ),
								  effistr[ ( int ) eff ] )
				  << std::endl;
	} while ( ( ++cpu, ++myAffinity ) ); // should reset affinity at the end …
	// output fresh knowledge
	std::cout << std::format( "- Physical cores: {:d}\n", coreIds.size() );
	if ( !effi.empty() )
	{
		std::cout << "- Performance core mask: " << perf.to_string() << std::endl;
		std::cout << "- Efficient cores mask : " << effi.to_string() << std::endl;
	} else std::cout << "- per Core efficiency class unknown.";
	return 0;
}
