# cpu_info Documentation - `affinity`

[→ back to overview](overview.md)

To be able to query all cpu's cpuid leafs, the executing process must be able to be bound to one specific cpu at the time of the `cpuid` instruction execution.

Every OS has a way to achieve this.  Most notably and widespread are "`cpu sets`".

This implementation aims to abstract the respective platform implementations inside into a platform independent interface: `affinity`.

---

## What is a cpu set exactly?

This question can be answered without a specific OS or architecture in mind.

A cpu set combines for example all available logical CPUs of a system into a grouped and enumerated hierarchy.

This can be done as simple as with a bit set assuming monotonous numerical ids starting at zero - which, btw., is the Linux way.

This example set would be the "full system cpu set".  Taken out only some logical cores of this set would produce a smaller set.  This smaller set could be used to restrict a thread to only be allowed to run on those cores (thread affinity).

A cpu set only consisting of one logical cpu is possible, as well. Many options are thinkable …

---

## implementation summary

This variant of cpu sets and helper classes is implemented header-only inside `cpu_info.hpp`, containing OS-agnostic implementations for Windows and Linux.

The base class describing a cpu set is `affinity`.

To be able to lock the current thread to a specific logical cpu core for a certain code block: `affinity_lock`.

Also, cycling all available logical cpu cores could be a requirement: `affinity_iterator`.

---

## class: `affinity`

### `affinity` constructor variants

```cpp
    affinity() noexcept;                    // create an empty cpu_set
    affinity( int logical_cpu ) noexcept;   // create a cpu_set with a single entry
```

### query current process

```cpp
    affinity &read_current() noexcept;      // queries the current process affinity,
                                            // applies it to itself, then returns a self-reference for chaining.
```

### applying a set

```cpp
    bool    set_to_current() const noexcept;
    bool    apply_next() noexcept;
```

Using `set_to_current()` does exactly what the name promises. It applies the `affinity` to the currently running thread.

This restricts the current thread to run on no other logical cores than the ones mentioned in this set.  If the set was empty - well, surprise!

( As far as development showed: on windows, that call would fail. On linux I guess the kernel is intelligent enough to either treat an empty set as if it was the "full system set", or fail gracefully, as well. )

Using `apply_next()` is discouraged, but safe.\
Its primary target is the `affinity_iterator`, thus this function WILL check if only one cpu is selected inside the `affinity`.\
In case none or more than 1 bits are set inside its mask, it won't do anything and return `false`.

> As a side-note: the function MUST check, else its termination condition would not safely be meetable, resulting in an infinite cpu switching loop.

---

### operations

There are operations to test an `affinity` for certain conditions:

- `bool empty()` / `operator bool()` (the 2 contradicting concepts)
- `size_t count_enabled() const noexcept` (count, how many cpus this affinity set will bind)

And there are operators to derive `affinity`s via logical operations between 2 sets:

- `affinity &   operator+=( int cpu_id )`     - add a single logical core to the set
- `affinity     operator|( const affinity& )` - join/unite two sets
- `affinity     operator&( const affinity& )` - intersect two sets
- `affinity     operator^( const affinity& )` - disjoin two sets (keep only non-shared items)

> ::Note:: Take care when setting parenthesis …

---

## class: `affinity_lock`

The affinity lock is most simple.  It is to be created on the stack as a local variable.

Upon explicit construction for a logical cpu number, the class binds the currently running thread to this cpu number and remembers the previously set `affinity`.

When scope leaves the 'current' block, it's destructor is invoked, restoring original thread affinity.

So, using the affinity lock is comparable to using a `std::scoped_lock< mutex >`:

```cpp
    {
        affinity_lock lock( 2 );    // lock to cpu #2
        do {
            // something
        } while( /* necessary */ );
    }
    // Original thread affinity restored.
```

> NOTE: this class is `non_copyable` and `non_movable` and `non_default_constructible`

---

## class `affinity_iterator`

Last but not least is the iterator class.  Nothing special:

- on construction: remember original thread `affinity`, bind to cpu #0
- on increment:
  - bind to next cpu number if exists, return `true`
  - in case current number was last, restore original `affinity`, return `false`
- on destruction: also restore original affinity (aborted run)

> Note: this class is `default_constructible`, `movable`, BUT `not_copyable`
> This is to restrict one active instance to one destruction-reset-of-affinity.

---

[← back to overview](overview.md)
