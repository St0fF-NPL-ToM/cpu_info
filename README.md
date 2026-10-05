# cpu_info

A small class for gathering CPU information, that may help finding answers about multithreading …

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

Please also have a look at the [docs folder](docs/overview.md).

***cpu_info:***

- declares `cpu_info` namespace,
- and implements the cpu_topo class

***cpu_enums_intel:*** … what that name says …

- declares X-macros: `CPU_FEATURES`, `CPU_DOMAINS`, `PROCESSOR_TYPE`, `CORE_TYPE`, `EFFICIENCY_TYPE`
- and respective bitfields / enumerations:
  - `cpu_feature` + accessor functions
  - `cpu_domain`, `cpu_processor_type`, `cpu_core_type`, `cpu_efficiency`

***cpu_id:***

- declares the `cpu_id` class:
  - public static member `cpuid( leaf, subleaf )` implements the `cpuid`-call (Windows/Linux)
- is a map of "all info of a single logical core we may get"
- contains specific query functions taylored to cpuid leafs
  - using `cpu_enums`, features, type, model, etc. can be queried

***cpu_set:***

- for managing task cpu affinities, this class tries to abstract over the respective platform interfaces:
  - linux: scheduler-interface via `cpu_set_t`
  - windows: processthreadsapi.h, processtopologyapi.h
- can query the system for current process affinity (default CTor operation)
- can be sat up empty or containing one single cpu number (Attn.: not apic_id!)
- provides operators:
  - join / intersect / dissect sets
  - add / remove cpus
- and last but not least: `applyToCurrentThread()` - which sets up the stored affinity mask for the current thread.

***cpu_info_types:***

- provides a few helper structs, types, and classes to ease some of the algorithms involved.

### Future

Now, with `cpu_set` and the efficiency features, thinkable stuff is using the `cpu_topo` as the management basis of a more complex application thread pool …

Also, the number of files steadily grew.  So another option would be to crunch it down into one header-only library.  Would make linking obsolete and ease usage even more.

---

## How to use

### build system import

There are probably many options how to use the code at hand. Most obvious are:

- in a <ins>*CMake build system*:</ins> use FetchContent / FindPackage and link the library:
  - please note how find_package arguments are passed through the `FetchContent_declare()` command …

```cmake
include(FetchContent)
FetchContent_declare( cpu_info
    GIT_REPOSITORY https://github.com/St0fF-NPL-ToM/cpu_info.git
    GIT_TAG development-0.0.3                                     # please choose appropriately
    GIT_SHALLOW on
    FIND_PACKAGE_ARGS PATHS ~/.local/lib64/cmake                  # local linux user install paths
                                                                  # very helpful for building locally!
)
FetchContent_makeAvailable( cpu_info )
```

> Note: this is only a guess, but steadily using e.g. `C:\Users\${user}\AppData\local` for your local Windows build's `CMAKE_INSTALL_PREFIX` may open up the same option on Windows systems!

- in *any other buildsystem* you may want to import cpu_info's source files into your source tree (currently):
  - cpu_info.h / cpp
  - cpu_info_types.hpp
  - cpu_enums_intel.h
  - cpu_id.h / cpp
  - cpu_set.hpp

> NOTES:
>
> - c++20 is required for compilation
> - nomenclature: if a cpp-header is self-contained, it shall be marked as a cpp header using ".hpp" extension\
>   <ins>note the special case</ins> "enums_intel": it serves as a traditional Header only declaring enumerations and X-macros, which does not produce any code, yet. This cannot be self-contained, as it is "nothing".
> - file amount / source structure may change without notice
>

---

### code use how-to

Inside your code, instantiate a `cpu_info::cpu_topo` object.  It will run a complete query of your CPU infrastructure upon construction:

- query current thread's cpu capabilities to determine operation mode
- cycle all CPUs to query their respective caps
  - all cpuid - leafs, including apic_id
  - this will bind the calling thread to each system cpu one after each other
  - taking some time to finish … so it's best to call it once and make the object globally available

Finally, you may use the class' members directly (it's mostly open, besides, you could edit it), or ask a question:

```cpp
	using namespace std;
	cpu_info::cpu_topo info;
	cout << format( "logical:  {:2d}\nphysical: {:2d}\nmodules:  {:2d}\n",
							info.countLevel( cpu_domain::LogicalDomain ),
							info.countLevel( cpu_domain::CoreDomain ),
							info.countLevel( cpu_domain::ModuleDomain ) );
	int i{};
	for (auto &id : info.cpu_ids)
		cout << format( "{:02d}: {:#06X}: '{:s}'\n", i++, (apic_id) id, id.brand_string() );
```

For further information, please consult the code itself and the [docs folder](docs/overview.md).

---

## Example(s)

Please activate `cpu_info_example` in your CMake Cache after cloning the source repository.
A simple example command line tool running on linux and Windows is included.
