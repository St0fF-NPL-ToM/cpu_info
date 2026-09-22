
#include <iostream>
#include "cpu_info.h"

using namespace std;

int main(int argc, const char *argv[])
{
	cpu_info brot;
	cout << "logical:  " << brot.countLevel(cpu_domain::LogicalDomain) << ",\n"
		 << "physical: " << brot.countLevel(cpu_domain::CoreDomain) << ",\n"
		 << "modules:  " << brot.countLevel(cpu_domain::ModuleDomain) << endl;
	int tpl{8};
	for (auto &id : brot.cpu_ids)
		cout << id << (--tpl ? ", " : (tpl = 8, "\n"));
	cout << endl;
	return 0;
}