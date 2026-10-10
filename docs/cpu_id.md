# cpu_info Documentation - cpu_id

[→ back to overview](overview.md)

The `cpu_id` class queries a single cpu core for all its cpuid leafs and subleafs.

A vector of those class instances will be produced by the `cpu_topo` constructor, containing one instance per logical cpu core present to the calling application.

To achieve its targets, there are some prerequisites:

- a platform independent implementation of the `cpuid` command
- some, also platform independent, way to retrieve the command's result
- also some POD types should be redefined to comply to cpp core guideline: "make it explicit"
- finally, "something to query" is needed - see [enumerations](enumerations.md) documentation

---

- [cpuid command and result type](#cpuid-command-and-result-type)
- [further support data types](#further-support-data-types)
- [cpu\_id internals](#cpu_id-internals)
  - [base design explanation](#base-design-explanation)
  - [explicit base overwrites - modifications](#explicit-base-overwrites--modifications)
  - [What is max\_leaf() ?](#what-is-max_leaf-)
  - [What is id\_leaf() ?](#what-is-id_leaf-)
  - [`APIC_id` by `cpu_domain`?](#APIC_id-by-cpu_domain)
  - [further query functionality](#further-query-functionality)
    - [`core_model()`?](#core_model)
    - [`brand_string()`?](#brand_string)
- [even further?](#even-further)

---

## cpuid command and result type

The Intel® `cpuid` instruction always returns 4 32-bit register values.  Thus, the return type was declared basically as an array of 4 unsigned integers.

To allow access in a way that better complies with the register names returned, a union is formed:

```cpp
	union cpuid_result
	{
		unsigned int r[ 4 ];
		struct
		{
			unsigned int ax, bx, cx, dx;
		} e;
	};
```

Which in turn is the result type of the platform-independent call:

```cpp
	static cpuid_result cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;
```

### where's the catch?

The catch is: depth of library use.  Maybe a user doesn't need all information - only the efficiency would suffice.  It wouldn't be good in that case, if the compiler produced all the available code...

Analysing this scenario:

- query functions work with `cpuid_results` by retrieving further leafs if necessary
- they call a function to receive a required leaf/subleaf
- this means: the `producer` of the result is free to be anything!

… leads to a simple solution:

- declare a "cpuid_accessor" requirement concept

```cpp
	template < class AT >
	concept cpuid_accessor = requires( const AT &accessor, unsigned leaf, unsigned subleaf ) {
		{ accessor.operator()( leaf, subleaf ) } -> std::convertible_to< cpuid_result >;
	};
```

- declare all query functionality templated based on that concept

```cpp
	template < class AT >
	ResultType function_name( <parameters…>, const AT& accessor_to_use );
```

- produce a simple struct meeting this requirement and implementing the actual cpu id instruction
  - userspace may instantiate this struct and materialize only those query functions needed by specializing the template at the call site
  - see `cpu_info_example`, which uses this approach
- be free to produce any other classes meeting this concept's requirement …
  - which is, what `cpu_id` is doing

### further support data types

When there are many "ID"s in play, one may get confused quite quickly.

Thus, some explicit typing was used to clarify the respective functionalities:

```cpp
	using APIC_id = unsigned;
	using id_mask = unsigned;
	using id_list = std::vector< APIC_id >;
```

---

## cpu_id internals

The interesting part about cpu_id is its base design, deriving from `std::map< unsigned, std::vector< cpuid_result > >`.

### base design explanation

The basic task is querying one cpu, *and all* information one cpu provides, comes in exactly this form:

- a sparsely filled list of leafs with their respective (monotonically increasing) subleafs.

Sparsely filled lists are nothing to easily work with, that's why associative arrays were born one day - so this is the reason for the outer shell as a map with the key `unsigned`, representing the leaf number.

Subleafs on the other hand - in case more than subleaf 0 exists - are continuous, thus a vector of subleafs is sufficient.

**By choosing this base to derive from, most management tasks just vanished, as they are simply inherited.**

Respectively, subtypes are declared (internally, prepend `cpu_id::` upon use) as:

```cpp
	using M = std::map< unsigned, std::vector< cpuid_result > >;
	using L = std::vector< cpuid_result >;
```

---

### explicit base overwrites / modifications

The default behaviour of `std::map` upon using the `operator[]`:

- default-create Value for queried Key, in case Key did not yet exist in the map

This behaviour is not intended, here.  As a `value` of this map is a `std::vector` and cpu_id "knows" which `Key`s may be valid, this access operator was simply made `protected`, so it cannot be called from outside the class.

---

### What is max_leaf() ?

`max_leaf()` returns the numeric id of the last leaf that exists on this particular cpu core.

This value is mostly useful if you want to use the queried leafs for more information, than cpu_id already provides.  Please adhere to [the Intel® documentation](https://cdrdv2.intel.com/v1/dl/getContent/671200) (see Volume 1, Chapter 21.3).

### What is id_leaf() ?

`id_leaf()` returns the numeric id of the cpuid-leaf that is used for identification (depends on cpu).

Possible values:

- `0x00` → invalid.  The object is empty.
- `0x01` → very old cpu, may be from the early 2k's
- `0x0b` → Core2-Era, I guess?
- `0x1f` → CoreI / Atom Era, current CPUs will return this …

---

### `APIC_id` by `cpu_domain`?

Every logical cpu core gets a unique APIC_id during system startup / cpu-hotplug.

Casting the `cpu_id` object to an `APIC_id` invokes the respective operator, returning the system unique apic-id describing this object.

Depending on the viewing perspective (e.g. which `cpu_domain`), a single core has different `APIC_id`s, some of which are shared with other cores (internal grouping), but the `LogicalDomain` id is this logical cpu core's system-unique `APIC_id`.

```cpp
	APIC_id            masked_id( cpu_domain domain ) const noexcept;
```

The returned values are generated by domain masking and domain shifting:

```cpp
	int	               domain_shift( cpu_domain domain ) const noexcept;
	id_mask            domain_mask( cpu_domain domain ) const noexcept;
```

---

### further query functionality

All further querying capabilities are expressed using the following functions:

```cpp
	bool			   operator()( cpu_feature feature ) const noexcept;
	uint8_t			   stepping() const noexcept;
	uint8_t			   family() const noexcept;
	uint8_t			   model() const noexcept;
	cpu_processor_type type() const noexcept;
	cpu_core_type	   core_type() const noexcept;
	unsigned		   core_model() const noexcept;
	string			   brand_string() const noexcept;
	cpu_efficiency	   efficiency() const noexcept;
```

#### `core_model()`?

The `core_model()` is a value that may be very different on each core of a single package, according to the documentation.  It is an internal stepping level of that specific core.

Seems like it really only serves informational value, therefore it is exposed as an unsigned value (24 bits are assigned by Intel®).

#### `brand_string()`?

Intel® offers multiple ways to discover the "cpu name", as it was sold to the owner.  It depends on the age, there are multiple methods to obtain a string for the cpu name from cpuid.  Which one method is to be used is defined within the cpuid results.  Thus, these strings will be produced upon query.

…

I believe all further queryable options are self-explanatory, or their purpose starts making sense with understanding the result [enumerations](enumerations.md).

---

## even further?

Regarding the future:

- querying Caches and TLBs could be made possible

---

[← back to overview](overview.md)
