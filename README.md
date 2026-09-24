# cpu_info

A small class for gathering CPU information, that most OS do not provide …

---

## Motivation

When designing and creating multithreaded applications, you run into the same issues over and over.

One issue might be: "spawning more worker threads than PHYSICAL cpu cores available introduces slow downs".

This actually is the most asked question in multithreading, at least of those questions I had to answer.

For a decade, simple CPUid was sufficient on desktop systems.  The next decade went with a 3-level-option.  Nowadays, it is indeed a good thing [vendors like intel provide example code](https://github.com/intel/SDM-Processor-Topology-Enumeration) to find those answers deterministically.

---

## How to use

There are probably many options how to use the code at hand. Most obvious are:
- use FetchContent and link the library
- import cpu_info.h and cpu_info.cpp into your build tree

> NOTE: c++20 is required for compilation due to the use of `std::format` and `std::vformat`

Inside your code, instantiate a `cpu_info` object.  It will run a complete query of your CPU infrastructure upon construction:

- query current thread's cpu capabilities to determine operation mode
- query apic_ids of system processors
- in case of "modern way available": cycle all CPUs to query their respective caps
	- this will bind the calling thread to each system cpu one after each other
	- taking some time to finish … so it's best to call it once and make the object globally available

Then you may use the class' members directly (it's mostly open, besides, you could edit it), or ask a question:

```cpp
	cpu_info info;
	cout << "logical:  " << info.countLevel( cpu_domain::LogicalDomain ) << ",\n"
	     << "physical: " << info.countLevel( cpu_domain::CoreDomain ) << ",\n"
	     << "modules:  " << info.countLevel( cpu_domain::ModuleDomain ) << endl;
	int tpl{8};
	for (auto &id : info.cpu_ids)
		cout << id << (--tpl ? ", " : (tpl = 8, "\n"));
	cout << endl;
```
