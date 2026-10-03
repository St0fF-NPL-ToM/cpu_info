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

## further support data types

## cpu_id internals

---

[→ back to overview](overview.md)
