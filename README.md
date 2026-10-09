# cpu_info

A small c++ namespace for gathering CPU information,\
that may help finding answers about multithreading …

---

## Motivation

When designing and creating multithreaded applications, you run into the same issues over and over.

One might be: "spawning more worker threads than PHYSICAL cpu cores available introduces slow downs".

This actually is the most asked question in multithreading, at least of those questions I had to answer.

On Intel-compatibles, long time a simple CPUid-call and some logics was sufficient on desktop systems. But as CPUs got more and more complex, the output of CPUID got more complex, too.
Therefore, it is indeed a good thing [vendors like intel provide example code](https://github.com/intel/SDM-Processor-Topology-Enumeration) to find those answers deterministically.

There are tons of tools to "show off" all your CPU capabilities on screen,\
but a real

    "can I do this ?"-class

I have not yet come across - this is `cpu_info`.

---

## Content

This mini-lib is evolving, as I am working on Windows and Linux, it just takes time.

### Current state

Please also have a look at the [docs folder](docs/overview.md).

***cpu_info:***

- source file `cpu_info.hpp`
- declares `cpu_info` namespace,
  - declares a lot of Intel-defined constants (in the form of X-macros):
    - `CPU_FEATURES`, `CPU_DOMAINS`, `PROCESSOR_TYPE`, `CORE_TYPE`, `EFFICIENCY_TYPE`
  - and respective bitfields / enumerations:
    - `cpu_feature` + accessor functions
    - `cpu_domain`, `cpu_processor_type`, `cpu_core_type`, `cpu_efficiency`
  - a concept for sourcing cpuid-results (so the instruction needs not be called if the result is already known)
  - template functions to acquire information from cpuid-results
  - the "call cpuid to get a result" - accessor structure implementing the CPUID instruction call
  - for restricting the current thread to a specific core and restoring affinity afterwards:
    - classes `affinity`, `affinity_iterator`, `affinity_lock`\
    → `affinity` encapsulates a process affinity and the ability to query, change and apply it to the current thread\
    → `affinity_iterator` simply binds to cpu #0 on construction, iterates until done, restores original affinity\
    → `affinity_lock` locks to a specific cpu number until destroyed (restores original affinity on destroy)

***cpu_id:***

- declares the `cpu_id` class - a `std::map` of "all info of a single logical core we may get"
- instantiates the template functions of `cpu_info` as member accessor functions
  - using `cpu_enums`, features, type, model, etc. can be queried

***cpu_topo:***

- declares the cpu_topo class - as a `std::vector` of all `cpu_id`s on the system
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

- in a <ins>*CMake build system*:</ins> use FetchContent / FindPackage and link the library:
  - please note how find_package arguments are passed through the `FetchContent_declare()` command …

```cmake
include(FetchContent)
FetchContent_declare( cpu_info
    GIT_REPOSITORY https://github.com/St0fF-NPL-ToM/cpu_info.git
    GIT_TAG development-0.0.3                                     # please choose appropriately
    GIT_SHALLOW on
    FIND_PACKAGE_ARGS PATHS ~/.local/lib64/cmake                  # local linux user install paths
                      COMPONENTS cpu_info cpu_id cpu_topo         # very helpful for building locally!
)
FetchContent_makeAvailable( cpu_info )
```

> Note: this is only a guess, but steadily using e.g. `C:\Users\${user}\AppData\local` for your local Windows build's `CMAKE_INSTALL_PREFIX` may open up the same option on Windows systems!

- The ANY OTHER WAY is simple: there are 3 header-only files, get those you need into your buildtree.

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
