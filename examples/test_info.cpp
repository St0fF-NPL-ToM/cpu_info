
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
	std::cout << brand_string() << std::endl
			  << std::format(
					 "- Family: {:#04x}, Model: {:#04x}, Stepping: {:#04x}\n- Logical cores : {:d}\n",
					 family(), model(), stepping(), cpu_info::count() );
	affinity			effi, perf;
	int					cpu{};
	std::set< APIC_id > coreIds;
	affinity_iterator	myAffinity{};
	do // What do we want to know about each single CPU?
	{
		// distinguish physical from logical cores
		coreIds.insert( apic_id( cpu_domain::CoreDomain ) );
		// build masks for efficient and performant cores
		const auto eff = efficiency();
		switch ( eff )
		{
			case cpu_efficiency::effficient: effi += cpu; break;
			case cpu_efficiency::performant: perf += cpu; break;
			default: break;
		}
		std::cout << std::format( "- Core #{:3d}: APIC_ID = {:#04x}, Efficiency: {:s}", cpu,
								  apic_id(), effistr[ eff ] )
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
