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
	- [explicit base overwrites](#explicit-base-overwrites)
	- [What is max\_leaf() ?](#what-is-max_leaf-)
	- [What is id\_leaf() ?](#what-is-id_leaf-)
	- [`apic_id` by `cpu_domain`?](#apic_id-by-cpu_domain)
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

Which in turn is the result type of the platform-independent call found as a static member function to `cpu_id`:

```cpp
	static cpuid_result cpuid( unsigned Leaf, unsigned Subleaf ) noexcept;
```

## further support data types

When there are many "ID"s in play, one may get confused quite quickly.

Thus, some explicit typing was used to clarify the respective functionalities:

```cpp
	using apic_id = unsigned;
	using id_mask = unsigned;
	using id_list = vector< apic_id >;
```

## cpu_id internals

The interesting part about cpu_id is its base design, deriving from `std::map< unsigned, std::vector< cpuid_result > >`.

### base design explanation

The basic task is querying one cpu, *and all* information one cpu provides, comes in exactly this form:

- a sparsely filled list of leafs with their respective (monotonically increasing) subleafs.

Sparsely filled lists are nothing to easily work with, that's why associative arrays were born one day - so this is the reason for the outer shell as a map with the key `unsigned`, representing the leaf number.

Subleafs on the other hand - in case more than subleaf 0 exists - are continuous, thus a vector of subleafs is sufficient.

By choosing this base to derive from, most management tasks just vanished, as they are inherited.

Respectively, subtypes are declared (internally, prepend `cpu_id::` upon use) as:

```cpp
	using M = map< unsigned, vector< cpuid_result > >;
	using L = vector< cpuid_result >;
```

---

### explicit base overwrites

You will find the following lines in the source:

```cpp
	cpu_topo::L& cpu_topo::operator[]( unsigned leaf ) noexcept
	{
		if ( contains( leaf ) ) return M::operator[]( leaf );
		else return _invalid;
	}
```

… as well as a const version of that operator.

This explicitly overwrites the `std::map`'s default behaviour upon using the `operator[]`: in case the key does not exist inside the map, it will be created and a non-const reference to this key's default-created value be returned.

This behaviour is not intended, here.  As a `value` of this map is a `std::vector` - return empty is the simplest option.

Anyhow, as a reference is returned, there has to be a static L instance as returned reference.

… simply hoping the user checks for emptiness instead of using it right away.

> In case somebody explains to me if it is possible to also provide non-noexcept variations, I'd go for an implementation.\
> In general I dislike the overhead of exception handling, thus I like to NOT put the user of my code into a situation, where a try-catch block was required.

You may as well see inside the code, that I also prefer to ***never early out***.\
Instead, every nesting level simply needs to "be there".

  This is **`honesty in coding`**.

Upon adding functionality to any function it makes you **NOT** *forget or oversee* those cases,\
that otherwise would have been early-outs.

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

### `apic_id` by `cpu_domain`?

Every logical cpu core gets a unique apic_id during system startup / cpu-hotplug.

Casting the `cpu_id` object to an `apic_id` invokes the respective operator, returning the system unique apic-id this object describes.

Depending on the viewing perspective (e.g. which `cpu_domain`), a single core has different `apic_id`s, some of which are shared with other cores (internal grouping), but the `LogicalDomain` id is this logical cpu core's system-unique `apic_id`.

```cpp
	apic_id            masked_id( cpu_domain domain ) const noexcept;
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

[→ back to overview](overview.md)
