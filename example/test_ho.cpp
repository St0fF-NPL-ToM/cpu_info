
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include <cpu_info.hpp>

int main( int argc, char* argv[] )
{
	std::cout << cpu_info::brand_string() << std::endl;
	return 0;
}
