# cpu_info

A small c++ namespace for gathering CPU information,\
that may help finding answers about multithreading …

---

## Motivation

When designing and creating multithreaded applications, the same issues arise over and over:

- spawning more worker threads than *physical* cpu cores available introduces slow downs
- binding threads to "thought-so" great selection of cores fails miserably (feels like sequentially computed)

This actually are the most asked questions in multithreading, at least of those questions I had to answer.

On Intel-compatibles, long time a simple CPUid-call and some logics was sufficient on desktop systems. But as CPUs got more and more complex, the output of CPUID got more complex, too.
Therefore, it is indeed a good thing [vendors like intel provide example code](https://github.com/intel/SDM-Processor-Topology-Enumeration) to find those answers deterministically.

There are tons of tools to "show off" all current CPU capabilities on screen,\
but a simple

    is this or that possible - class

I have not yet come across - this motivated `cpu_info`.  Well, not exactly a class, but a library with a namespace …

---

## Basic idea

Have "something" usable in c++ that can differentiate the cpu topology of a system so potentially arising questions like those mentioned before can be answered.

Pursuing this "idea" I found the [Intel-Code](https://github.com/intel/SDM-Processor-Topology-Enumeration). Reading it and immediately seing:

- this code was used 20 years ago to query if SSE4.2 is available
- and there is, how it all evolved …

So the only problem was: this C program shows it all, but is not "something usable in c++".

It helped evolve that `idea`:

- provide a C++ "thing" (library, header-only-lib)
- should be able to read and interpret current cpuid data
- arising requirements:
  - thread affinity: cpuid-calls are "per logical core"
  - a shitload of flags and enumerations, which make up cpuid
- should have an intuitive, possibly self-explaining API

### Current state

Please also have a look at the [docs folder](docs/overview.md).

***cpu_info.hpp:*** → declares `cpu_info` namespace

- declares a lot of Intel-defined constants (in the form of X-macros):
  - `CPU_FEATURES`, `CPU_DOMAINS`, `PROCESSOR_TYPE`, `CORE_TYPE`, `EFFICIENCY_TYPE`
- and respective bitfields / enumerations:
  - `cpu_feature` + accessor functions
  - `cpu_domain`, `cpu_processor_type`, `cpu_core_type`, `cpu_efficiency`
- a concept for sourcing cpuid-results `cpuid_result accessor` (so the instruction needs not be called more than once per core+leaf+subleaf)
- some necessary type (re)definitions
- template functions to acquire information from cpuid-results (using such a `cpuid_result accessor`)
- the `CPUID` structure (being such a `cpuid_result accessor`), implementing the actual instruction call
- for restricting the current thread to a specific core and restoring affinity afterwards:
  - classes `affinity`, `affinity_iterator`, `affinity_lock`\
  → `affinity` encapsulates a process affinity and the ability to query, change and apply it to the current thread\
  → `affinity_iterator` simply binds to cpu #0 on construction, iterates until done, restores original affinity\
  → `affinity_lock` locks to a specific cpu number until destroyed (restores original affinity on destroy)

***cpu_id.hpp:*** → declares the `cpu_id` class - a `std::map` of "all info of a single logical core we may get"

- this class acts as a `cpuid_result accessor` and reads all available leafs and subleafs during construction
- instantiates the template functions of `cpu_info` as member accessor functions
- using `cpu_enums`, features, type, model, etc. can be queried

***cpu_topo.hpp:*** → declares the cpu_topo class - as a `std::vector` of all `cpu_id`s on the system

- uses the `affinity_iterator` during refresh to parse the whole topology
- delivers answers like "how many physical cores do exist?" (counting the different domains of this topology)
- or "does this cpu have a hybrid architecture, and if so, which core is of which type?"
  - which can already be queried with the namespace only, but as `cpu_topo` gathers a list of all `cpu_id`s, it operates on present data and is thus better suited for repeated tasks, like controlling a thread pool

### Future

The close future is: as soon as the API is stable, there will be a 0.5 version bump with a first "complete" release.

---

## How to use

### build system import

There are probably many options how to use the code at hand. Most obvious are:

- import the `hpp` files you need into your buildtree
  - cpu_id.hpp depends on cpu_info.hpp
  - cpu_topo.hpp depends on cpu_id.hpp
- in a <ins>*CMake build system*:</ins> use FetchContent / FindPackage and link the library:

```cmake
include(FetchContent)
FetchContent_declare( cpu_info
    GIT_REPOSITORY https://github.com/St0fF-NPL-ToM/cpu_info.git
    GIT_TAG 0.0.4                                                 # please choose appropriately
    GIT_SHALLOW on
    FIND_PACKAGE_ARGS PATHS ~/.local/lib64/cmake                  # local linux user install paths
                                                                  # very helpful for building locally!
)
FetchContent_makeAvailable( cpu_info )
```

> Note: The argument passing using `FIND_PACKAGE_ARGS` allows to include a local installation of the library.
> (in this example: local on a linux system)
> This is only a guess, but steadily using e.g. `C:\Users\${user}\AppData\local` for your local Windows build's `CMAKE_INSTALL_PREFIX` may open up the same option on Windows systems. In that case, it would produce some NIX'like structure inside "AppData\local".

> NOTES:
>
> - c++23 is required for compilation
> - nomenclature: if a cpp-header is self-contained (e.g. no TU needed), it shall be marked as a cpp header using ".hpp" extension
>   - obviously, this was achieved with cpu_info …
> - file amount / source structure may change without notice

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

Please activate `CPU_INFO_BUILD_EXAMPLES` in your CMake Cache after cloning the source repository.
A simple example command line tool running on linux and Windows is included for all 3 depths of the library.
