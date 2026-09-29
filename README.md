# cpu_info

A small class for gathering CPU information, that seems missing from the cpp std libs …

---

## Motivation

When designing and creating multithreaded applications, you run into the same issues over and over.

One issue might be: "spawning more worker threads than PHYSICAL cpu cores available introduces slow downs".

This actually is the most asked question in multithreading, at least of those questions I had to answer.

On Intel-compatibles, for a decade, simple CPUid was sufficient on desktop systems.  The next decade went with a 3-level-option.  Nowadays, it is indeed a good thing [vendors like intel provide example code](https://github.com/intel/SDM-Processor-Topology-Enumeration) to find those answers deterministically.

Now, there is CPP20 and CPP23 with a lot of multithreading-helpers.  But it doesn't go as deep as affinity selection.  In other words: we get the playthings to create, but have to wait for those other playthings to optimize.

---

## Content

This mini-lib is evolving, as I am working on Windows and Linux, it just takes time.

### Current state

***cpu_info:***

- declares `cpu_info` namespace,
- external interface functions,
- and implements the cpu_topo class

***cpu_enums:*** … what that name says …

- declares X-macros: `CPU_FEATURES`, `CPU_DOMAINS`, `PROCESSOR_TYPE`, `CORE_TYPE`, `EFFICIENCY_TYPE`
- and respective bitfields / enumerations:
  - `cpu_feature` + accessor functions
  - `cpu_domain`, `cpu_processor_type`, `cpu_core_type`, `cpu_efficiency`

***cpu_id:***

- declares the `cpuid_result` and implements the `cpuid`-call (Windows/Linux)
- declares and implements `cpuid_leafs` as "all info of a single logical core we may get"
- assembles this into a `cpu_id` class with specific query functions taylored to cpuid leafs
  - using `cpu_enums`, features, type, model, etc. can be queried
- declares some helper structures

***cpu_set:***

- for managing task cpu affinities, this class tries to abstract over the respective platform interfaces:
  - linux: scheduler-interface via `cpu_set_t`
  - windows: processthreadsapi.h, processtopologyapi.h
- a default-constructed `cpu_info::cpu_set` object will contain a "current process' CpuSet"
- CTor for single cpu affinity also provided
- as well, as operators:
  - join / intersect sets
  - add / remove cpu-ids
- and last but not least: `applyToCurrentThread()` - which sets up the stored affinity mask for the current thread.


### Future

Plans are, that this is not the end. As can be seen from the public (nyi) interface:

```cpp
		id_list		  optimalProcessAffinity( int thread_count, bool prefer_performance = true );
```

A `std::vector<unsigned>` being passed back and forth may not be the best solution. But the "set cpu affinity"-functions were already existing.

Now, with `cpu_set` and the efficiency features, even more is possible...

---

## How to use

There are probably many options how to use the code at hand. Most obvious are:

- use FetchContent and link the library
- import cpu_info's source files into your source tree (currently):
  - cpu_info.h / cpp
  - cpu_id.h / cpp
  - cpu_enums.h
  - cpu_set.h / cpp

> NOTE: c++20 is required for compilation due to the use of `std::format` and `std::vformat`

> NOTE: file amount / source structure may change without notice

Inside your code, instantiate a `cpu_info` object.  It will run a complete query of your CPU infrastructure upon construction:

- query current thread's cpu capabilities to determine operation mode
- cycle all CPUs to query their respective caps
  - all cpuid - leafs, including apic_id
  - this will bind the calling thread to each system cpu one after each other
  - taking some time to finish … so it's best to call it once and make the object globally available

Finally, you may use the class' members directly (it's mostly open, besides, you could edit it), or ask a question:

```cpp
	using namespace std;
	cpu_info::cpu_topo info;
	cout << "logical:  " << info.countLevel( cpu_domain::LogicalDomain ) << ",\n"
	     << "physical: " << info.countLevel( cpu_domain::CoreDomain ) << ",\n"
	     << "modules:  " << info.countLevel( cpu_domain::ModuleDomain ) << endl;
	int tpl{8};
	for (auto &id : info.cpu_ids)
		cout << ( string ) id << (--tpl ? ", " : (tpl = 8, "\n"));
	cout << endl;
```

---

## Example(s)

Please activate `cpu_info_example` in your CMake Cache after cloning the source repository.
A simple example command line tool running on linux and Windows is included.

Tested on:
- Windows 11 Home
- Fedora Linux 44
