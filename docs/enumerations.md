# cpu_info documentation - enumeration types

Most enumerations in this project are declared in the form of an X-macro, first, then transmogrified into the respective enum or enum class (depending on usage pattern).

The intention: to be able to easily provide a string table of any kind for names, so creating output functions poses no challenge.

## Overview

| enum class | X-macro | X-params | description |
| :--- | :---: | :---: | :--- |
| [`cpu_features`](#cpu_infocpu_features) | CPU_FEATURES | NAME, REG, BIT, LEAF, SUB | contains all Intel®-defined feature bits retrievable via cpuid.|
| [`cpu_domain`](#cpu_infocpu_domain) | CPU_DOMAIN | NAME | for topology and queries, the domain of a cpu core as described by Intel®<br/>(e.g. `LogicalDomain`, `CoreDomain` (physical cores), `DieDomain`, etc.) |
| [`cpu_core_type`](#cpu_setcpu_core_type) | CORE_TYPE | NAME, MASK_VALUE | A 6 bit mask, where only 2 values are really useful: `Core` and `Atom` (other values should never be encountered, or treated as "reserved, invalid") |
| [`cpu_efficiency`](#cpu_infocpu_efficiency) | EFFICIENCY_TYPE | NAME, VALUE | Translation of `cpu_core_type` into its actual meaning |
| [`cpu_processor_type`](#cpu_setcpu_processor_type) | PROCESSOR_TYPE | NAME | not really of importance anymore, a relatively old cpuid-bitmask containing `OEM_processor`, `IntelOverDrive`, `Dual_processor`, … |
| [`cpu_set::init_type`](#cpu_setinit_type) | none defined | … | Cookie-type to control parameter-less cpu_set constructor behaviour. |

Starting out with the easy ones …

---

## cpu_info::cpu_efficiency

Describes the view of a single logical core on itself.

**Values:**

- `unknownEff`: 0
- `effficient`: 1
- `performant`: 2

---

## cpu_set::init_type

This enum is only used in the `cpu_set::cpu_set( init_type )`-constructor.

A "No parameter"-CTor (i.e. "default") can only be implemented once. But I needed a 2nd parameter-less CTor for the class to produce an empty set.\
On such occasions, modern C++ should introduce a "cookie-type" (or "key type") to control behaviour:

**Values:**

- `empty`: create an empty set
- `process`: create a set from querying current process' state

Thus, the default "parameterless" CTor will query the current process affinity, while a call to `cpu_set::cpu_set( init_type::empty )` will indeed return an empty set.

***ATTENTION:*** on Windows, an application is bound to a single cpu group, unless explicitly configured differently.  Most desktop systems have only one cpu group, but in case there were more, this group id is necessary!

Thus, the "create empty set" CTor call will also query the system on Windows. It keeps the group id and discards the process affinity mask.

---

## cpu_set::cpu_core_type

This enum represents values directly taken out of leaf 0x1a of cpuid.

If you like, please use the X-macro for creating an output string list and output as you wish.

In any other senseful scenarios, please use the `cpu_efficiency` enumeration instead.

---

## cpu_set::cpu_processor_type

This enum represents values directly taken out of cpuid.  The values seem purely informational.

If you like, please use the X-macro for creating an output string list and output as you wish.

---

## cpu_info::cpu_domain

This value is predefined by Intel® to contain the different levels of cpu core hierarchy.

It is used to determine the topology in `cpu_topo`.

It can be used to query topology: level-masked and shifted `apic_id` of cpu cores, as well as the amount of participants of each level, as well as each masked apic_id.

Examples:

```cpp
    cpu_info::cpu_topo    info;
    const auto logical  = info.countLevel( cpu_info::cpu_domain::LogicalDomain );
    const auto physical = info.countLevel( cpu_info::cpu_domain::CoreDomain );
    const auto modules  = info.countLevel( cpu_info::cpu_domain::ModuleDomain );

    // core-domain-id of cpu[ph-1] (physical count should be at max cpu count)
    const auto cd_id    = info[ physical-1 ].id( cpu_info::cpu_domain::CoreDomain );

    // retrieve mask name value of a domain:
    const id_mask mask  = info.level_masks_names[ cpu_info::cpu_domain::CoreDomain ].first;
    const std::string n = info.level_masks_names[ cpu_info::cpu_domain::CoreDomain ].second;
```

As seen in that last example, there is no need to use the X-macro of cpu_domain to get visual output data, it's already done inside cpu_topo, at least for all domains currently in use.

> I actually do not remember why, this should be subject to inspection!  During build-up of the topology, that string is not needed at all and with this member, the X-macro-style definition does make little sense on `cpu_domain`.

---

## cpu_info::cpu_features

This enum is an exhaustive conglomeration of Intel®-defined bits of cpu features.

Internally, its numeric value is decomposed to look up the current state inside the cpuid-leafs and subleafs.

So its main purpose is querying, if a specific core supports a specific feature.
> Indeed, features may differ between cores on a physical die or package!

### hints on using the X-macro

The features-enumeration is non-monotonical.  Thus you cannot simply create a list of strings from it in case you desire specific output.

The X-Macro signature is: `X( NAME, REG, BIT, LEAF, SUB )`. Thus, to create a map of strings, you could use:

```cpp
#define X( NAME, ... ) { cpu_info::cpu_features::#NAME, ##NAME },
    static const unordered_map< cpu_info::cpu_features, std::string > map{ CPU_FEATURES( X ) };
#undef X
```
