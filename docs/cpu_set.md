# cpu_info Documentation - cpu_set

[→ back to overview](overview.md)

To be able to query all cpu's cpuid leafs, the executing process must be able to be bound to one specific cpu at the time of the `cpuid` instruction execution.

Every OS has a way to achieve this.  Most notably and widespread are "CPU SETS".

This implementation aims to abstract the platform implementation inside into a platform independent interface.

---

## What is a cpu set exactly?

This question can be answered without a specific OS or architecture in mind.

A cpu set combines for example all available logical CPUs of a system into a grouped and enumerated hierarchy.

This can be done as simple as with a bit set assuming monotonous numerical ids starting at zero - which, btw., is the Linux way.

This example set would be the "full system cpu set".  Taken out only some logical cores of this set would produce a smaller set.  This smaller set could be used to restrict a thread to only be allowed to run on those cores (thread affinity).

A cpu set only consisting of one logical cpu is possible, as well. Many options are thinkable …

## implementation details

This variant of cpu sets is implemented as a header-only class, containing OS-agnostic implementations for Windows and Linux.

---

### constructor variants

```cpp
    cpu_set( init_type _ = init_type::process ) noexcept;   // query the current process' cpu_set
    cpu_set( init_type::empty ) noexcept;                   // create an empty cpu_set
    cpu_set( int logical_cpu ) noexcept;                    // create a cpu_set with a single entry
```

After primary implementation I realized, that creating an empty set would also make a lot of sense.

That way, the parameter-less default CTor started to get two different meanings. As I did not want to resolve this to a named boolean parameter with default being set to "query process", I decided to create the cookie enum class `cpu_info::init_type`.

This is indeed the same option drawn, but slightly more conformant to the cpp core guidelines: make things explicit.

The default stays `init_type::process` - so the default CTor resolves to querying.

In case an empty set shall be constructed, it must be explicitly hinted via `init_type::empty` to the CTor.

---

### applying a set

Using `bool applyToCurrentThread() const noexcept;` does exactly what the name promises. It applies the `cpu_set` instance to the currently running thread.

This restricts the current thread to run on any other logical cores than the ones mentioned in this set.  If the set was empty - well, surprise!

( As far as development showed: on windows, that call would fail. On linux I guess the kernel is intelligent enough to either treat an empty set as if it was the "full system set", or fail gracefully, as well. )

---

### operations

The class implements all operators that make some kind of sense:

- `operator+( int cpu_id )`     - add a single logical core to the set
- `operator-( int cpu_id )`     - remove a single logical core from the set
- `operator|( const cpu_set& )` - join/unite two sets
- `operator&( const cpu_set& )` - intersect two sets
- `operator^( const cpu_set& )` - disjoin two sets (keep only non-shared items)

All those operators are also implemented as respective modify-writes (in-place operators), returning a self-reference for chaining modifications together.

> ::Note:: Take care when setting parenthesis …

---

[← back to overview](overview.md)
