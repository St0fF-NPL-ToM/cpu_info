# cpu_info Documentation - enumeration types

[→ back to overview](overview.md)

Most enumerations in this project are declared in the form of an X-macro, first, then transmogrified into the respective enum or enum class (depending on usage pattern).

The intention: to be able to easily provide a string table of any kind for names, so creating output functions poses no challenge.

## Overview

| enum class | X-macro | X-params | description |
| :--- | :---: | :---: | :--- |
| [`cpu_features`](#cpu_infocpu_features) | CPU_FEATURES | NAME, REG, BIT, LEAF, SUB | contains all Intel®-defined feature bits retrievable via cpuid. |
| [`cpu_domain`](#cpu_infocpu_domain) | CPU_DOMAIN | NAME | for topology and queries, the domain of a cpu core as described by Intel®<br/>(e.g. `LogicalDomain`, `CoreDomain` (physical cores), `DieDomain`, etc.) |
| [`cpu_core_type`](#cpu_setcpu_core_type) | CORE_TYPE | NAME, MASK_VALUE | A 6 bit mask, where only 2 values are really useful: `Core` and `Atom` (other values should never be encountered, or treated as "reserved, invalid") |
| [`cpu_efficiency`](#cpu_infocpu_efficiency) | EFFICIENCY_TYPE | NAME, VALUE | Translation of `cpu_core_type` into its actual meaning |
| [`cpu_processor_type`](#cpu_setcpu_processor_type) | PROCESSOR_TYPE | NAME | not really of importance anymore, a relatively old cpuid-bitmask containing `OEM_processor`, `IntelOverDrive`, `Dual_processor`, … |

Starting out with the easy ones …

---

## cpu_info::cpu_efficiency

Describes the view of a single logical core on itself.

**Values:**

- `unknownEff`: 0
- `effficient`: 1
- `performant`: 2

---

## cpu_set::cpu_core_type

This enum represents values directly taken out of leaf 0x1a of cpuid.

If you like, please use the X-macro for creating an output string map and output as you wish:

```cpp
#define X( n, t ) { t, #n },
	map< int, string > core_types{ { CORE_TYPE( X ){ 0, "NONE" } } };
#undef X
```

> P.s.: the addition of the "0"-element just serves security. If `cpuid leaf 0x1a` was not present, zero may be a returned value.
>
> Another variation would be to transform the value from its 7 bits into 2 (`v >> 5`), as only values `0x20` and `0x40` are to be expected if that cpuid leaf exists, and `0x00` if it doesn't exist.
>
> Any other values are by specification *RESERVED*, so it is not to be expected until the specification changes in that point.

In any other senseful scenarios, please use the `cpu_efficiency` enumeration instead.

---

## cpu_set::cpu_processor_type

This enum represents values directly taken out of cpuid.  The values seem purely informational:

    #define PROCESSOR_TYPE( X )	X( OEM_processor ) X( IntelOverDrive ) X( Dual_processor ) X( Intel_reserved )

If you like, please use the X-macro for creating an output string list and output as you wish.

```cpp
#define X( n ) #n,
	constexpr const char* processor_types[] = { PROCESSOR_TYPE( X ) };
#undef X
```

---

## cpu_info::cpu_domain

This value is predefined by Intel® to contain the different levels of cpu core hierarchy:

- InvalidDomain = 0
- LogicalDomain = 1
- CoreDomain …
- ModuleDomain
- TileDomain
- DieDomain
- DieGrpDomain

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

    // retrieve mask / shift value of a domain: (this should be shared - i.e. same output across all cores!)
    const id_mask mask  = info.level_mask( cpu_info::cpu_domain::CoreDomain );
    const int shift     = info.level_shift( cpu_info::cpu_domain::CoreDomain );
```

If you like, please use the X-macro for creating an output string list and output values as you wish (see [processor type](#cpu_setcpu_processor_type) for X-macro usage example).

---

## cpu_info::cpu_features

This enum is an exhaustive conglomeration of Intel®-defined bits of cpu features.

Internally, its numeric value is decomposed to look up the current state inside the cpuid-leafs and subleafs.

So its main purpose is querying, if a specific core supports a specific feature.
> !ATTN: Indeed, features may differ between cores on a physical die or package!

### hints on using the X-macro

The features-enumeration is non-monotonical.  Thus you cannot simply create a list of strings from it in case you desire specific output.

The X-Macro signature is: `X( NAME, REG, BIT, LEAF, SUB )`. Thus, to create a map of strings, you could use:

```cpp
#define X( NAME, ... ) { cpu_info::cpu_features::#NAME, ##NAME },
    static const map< cpu_info::cpu_features, std::string > map{ CPU_FEATURES( X ) };
#undef X
```

---

[← back to overview](overview.md)
