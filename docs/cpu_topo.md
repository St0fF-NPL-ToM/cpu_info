# cpu_info Documentation - cpu_topo

[→ back to overview](overview.md)

The cpu topology of a system is mostly of informational nature.  But when it comes to scheduling and optimizing many tasks at hand, taking this topology into account allows for much more flexibility and thoughput.

Modern CPUs and OSes are vastly parallel, not much left to compare to a CMOS 6502 from the nineteenseventies …

They allow for multiple types of mulitple cpus on a single chip nowadays, so the first glance informational nature quickly becomes inevitable system structure.

---

## cpu topology class

The basic questions I was asking when starting with the implementation was:

- how many threads do make sense
- does the system support different cores?
  - if yes how many of which type? (to better answer the first question)
- which cores are shared logicals and which ones are pure physical?

The overall question just being: how can I spread different kinds of tasks optimally on the system at hand?

Therefore the `cpu_topo` constructor does the following things in order:

- queries the active `cpu_set` of the process
- queries the logical cpu count
- in a loop: binds the calling thread to each logical core and obtains all cpuid leafs at once
- parses the topology from the cpu_id's
- re-applies the process affinity mask

This construction process can take a considerate amount of time depending on the number of logical cpus available and the context switching overhead necessary to change the process affinity.

It is therefore ***recommended*** to create a single instance of `cpu_topo` and make it globally available to your code - or any means of personal choice to make sure only a single instance is created.

---

### class contents

After creation, this class serves as an informational container.

Nowadays there is indeed CPU-hotplugging possible - in case hot-plugging is supported, there is the `refresh()` function to be called.  It simply clears internal state an repeats the initialization procedure.

The members are:

```cpp
    const cpu_set       process_affinity;   // contains the first queried (i.e. orignal) process affinity
    vector< cpu_id >    cpu_ids;            // the vector of `cpu_id` entries for each logical cpu (numerical
                                            // (non-apic-id) ID of each core is its index into the vector)
    vector< map< apic_id, int > >
                        lvl_ids;            // A vector of maps, used to count specific apic ids (masked ids)
                                            // The `countLevel` function uses these entries.
```

- `level_masks_names` - maps currently available cpu_domains onto its apic_id_mask and domain name
  - this member is subject to removal (see issue #4)

For querying, those functions are available:

```cpp
    size_t          count()                         const noexcept
    size_t          countLevel( cpu_domain lvl )    const noexcept
    const cpu_id &  operator[]( size_t index )      const noexcept
    bool		    knowsEfficiency()               const noexcept
```

---

### class internals

There is not much to be said.

The class does all its "heavy lifting" inside the `refresh()` function, which (re)creates the cpu_id list by switching thread affinity.

After restoring the original affinity, it calls the protected member function `parse_topology`, which is modeled after [Intel®'s excellent C example](https://github.com/intel/SDM-Processor-Topology-Enumeration).

This is the reason we're carrying around structures like `apicid_bit_layout` inside `cpu_id.h` - where it definitively not belongs.  See issue #7.

---

[→ back to overview](overview.md)
