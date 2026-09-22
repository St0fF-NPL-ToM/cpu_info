
#include <iostream>
#include "cpu_info.h"

using namespace std;

int main(int argc, const char *argv[])
{
	cpu_info brot;
	cout << "logical:  " << brot.countLevel(cpu_domain::LogicalDomain) << ",\n"
		 << "physical: " << brot.countLevel(cpu_domain::CoreDomain) << endl;
	return 0;
}