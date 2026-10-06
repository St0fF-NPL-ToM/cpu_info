# cpu_info - Documentation overview

This is the root of `cpu_info`'s documentation.

`cpu_info` was originally intended to answer a few questions:

> With those current cpu architectures that have "performance cores" and "efficient cores":\
> *Shouldn't a thread pool take this "feature" into account?*\
> *How do I find out about the feature (platform independently)*?

My personal work is targeting x86-compatible desktop systems running Linux or Windows.

Thus, it was simple to decide - also for some brain-work of mine - to directly interpret the cpuid instruction, as that is available due to the hardware requirement.

I am aware, that this was a *wheel reinvention* in some way.  Windows and Linux as well provide all that information in some way, so platform independence would simply be a question of two separate code paths.

( Of which there are, anyhow, btw. )

---

## `cpu_info` namespace

The `cpu_info` namespace enrolls the library.  Every piece is contained within the namespace.

---

### Classes

- [`cpu_topo`](cpu_topo.md)
  - a capture of the current system's cpu topology and all core's cpuid information available
  - ***ATTN!:*** best to use a global static instance, so it is queried once upon startup
- [`cpu_id`](cpu_id.md)
  - describes one single core of the running system (cpuid-leafs and -subleafs)
  - provides the cpuid-instruction execution platform implementations as a static member function
  - provides lots of "core query" functionality
- [`cpu_set`](cpu_set.md)
  - a platform-independent abstraction of a simplified cpu affinity set
  - can be used to query the current process affinity, or build and set an affinity for the current thread

---

### [Enumerations](enumerations.md)

Most enumerations in this project are declared in the form of an X-macro. The intention is to be able to easily provide a string table of any kind for names, so creating output functions poses no challenge.

We have a few [`cpu_enumerations`](enumerations.md) for behavioural control, as well as for feature and type querying:

| enum class | X-macro | X-params | description |
| :--- | :---: | :---: | :--- |
| `cpu_features` | CPU_FEATURES | NAME, REG, BIT, LEAF, SUB | contains all Intel®-defined feature bits retrievable via cpuid. |
| `cpu_domain` | CPU_DOMAIN | NAME | for topology and queries, the domain of a cpu core as described by Intel®<br/>(e.g. `LogicalDomain`, `CoreDomain` (physical cores), `DieDomain`, etc.) |
| `cpu_core_type` | CORE_TYPE | NAME, MASK_VALUE | A 6 bit mask, where only 2 values are really useful: `Core` and `Atom` (other values should never be encountered, or treated as "reserved, invalid") |
| `cpu_efficiency` | EFFICIENCY_TYPE | NAME, VALUE | Translation of `cpu_core_type` into its actual meaning |
| `cpu_processor_type` | PROCESSOR_TYPE | NAME | not really of importance anymore, a relatively old cpuid-bitmask containing `OEM_processor`, `IntelOverDrive`, `Dual_processor`, … |
| `cpu_set::init_type` | none defined | … | Cookie-type to control parameter-less cpu_set constructor behaviour. |

---

### [Debugging helpers](debugging.md)

Well, have a look there. Helpers are provided as:

- natvis
- lldb script

---
