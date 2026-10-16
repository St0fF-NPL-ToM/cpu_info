# cpu_info Documentation - EfficientCores

[→ back to overview](overview.md)

---
<!-- TOC -->

- [Efficient Cores and Thread Pools](#efficient-cores-and-thread-pools)
    - [efficient vs. performant cores - a tech perspective](#efficient-vs-performant-cores---a-tech-perspective)
        - [Conclusion](#conclusion)
- [Thread Pool](#thread-pool)
    - [planning: TASK_GROUPS](#planning-task_groups)
        - [group creation](#group-creation)
        - [group storage](#group-storage)
    - [planning: pool design](#planning-pool-design)
        - [pool design - storage](#pool-design---storage)

<!-- /TOC -->
---

## Efficient Cores and Thread Pools

### efficient vs. performant cores - a tech perspective

Somehow I feel the need to write down a few sentences about "efficient vs. performant cores".

So, this Intel® microarchitecture took that step to ship *"some big CORE-Is"* as well as *"some smaller Atom®s"* on a single socket CPU.

`cpu_info` can distinguish, in case this feature is supported.

**You might think:**
"Wow, this is cool - so computation can be bound to performant cores and a USB-listener goes to the Atom …"

Indeed, wouldn't that be cool?  That's what Intel's engineers thought, also, despite it's not quite right anyhow.

Under the hood, Intel also introduced a lot of much cooler technologies most major OS kernels already use.

These technologies already make sure, threads of execution are scheduled to the most fitting core available. This works via some kind of performance reporting, which hints back to the OS, where to optimally schedule previously executed code.
As with all techs that base on feedback: one or two runs are needed until it works fine.

If userspace was to pin a specific thread to a specific core, this will be honoured by the OS.
But it takes that decision off the OS's "plate" and probably even violates the CPU's own reporting-results.
In case the OS decided one click earlier to use that explicit core with another thread of execution, it would also be delayed - thus the explicit binding becomes counter-productive.

Pinning multiple cores on the other hand would eliminate aforementioned problem, but most likely in many cases still violate the feedback results, thus disturb the OS scheduling process.

***Facing reality:***

- it needs to take special precautions to not schedule "our thread" on a disallowed core
- there is overhead in pinning threads / not using inherited affinity
- the only one or two situations the OS scheduler would probably schedule our thread on a core we don't want it to run on are:
  - right at the first run(s),
  - maybe even only the first few context switches.
  - in case we wanted it to run on an Atom, but a CoreI's 2nd HT is still free
  → OS would most likely keep the Atom in sleep state and schedule on that free performance HT-core.

#### Conclusion

The conclusion is simple.  We expect to optimize our execution flow by introducing static knowledge.

We saw:

- that static knowledge is only useful if run only once, maybe not even then
- other tech makes sure a thread runs on the right core(s) most of the time (*"most"*: like >>99%)
- managing affinities introduces overhead
  - see above for OS-related
  - topology detection and affinity management - even if only 2 groups would be created - would clobber the code

**It does not make any sense to introduce core `performance`/`efficiency` affinities into a modern thread pool.**

**The only hard restriction making at least some sense: provide (CoreCount-1) running threads, so your app does not over-provision the system.**

**That way YOU schedule your application, and the OS schedules your app into the system.**

---

## Thread Pool

We learned in the previous paragraphs, that a modern thread pool should not waste resources, that are already preoccupied by the OS.

But what are the needs?

- be able to `run a thread and receive a future` - c++23, `RUN_ONCE`
- maybe for repeated parallel activities: `TASK_GROUPS`
  - e.g.: "framely update operations"
    - are added to the GROUP once (set & forget)
    - repeated many times upon a trigger: `run_group`
  - option to run another task as part of a GROUP: `run_in_group`
  - option to check/wait for a whole GROUP: `await_group`, `group_running_tasks`

---

### planning: TASK_GROUPS

So there are the needs - let's deep-dive that `TASK_GROUPS` idea.

An application oftentimes has some background tasks to be done repeatedly.  I personally call those tasks the `framelies`. But they don't need to be as frequent as "framely" - they could be even more or much less frequenfly needed.

Anyhow - let's not see internal task synchronization here, that would be userspace's part.  We see the option with more potential: **await multiple conditions at once**.

That is, what your application orchestrator will be interested in at some point: are those tasks done?

#### group creation

Easiest is to "tell on construction", how many groups will be needed. Thinking about opposite claims/needs brings up extensibility:

- maybe you want to support "plug-ins"
- you'd like to give those plugins the same options your application has
**→ this would be the time to "add a new group"**

This thought makes everything even more simple:

- Groups take a simple size_t `ID`
- Groups are added on demand/by adding tasks
- in this use-case Group-Removal on demand also seems to make sense

So, the introducing line is already flipped. On construction, there are no groups.

#### group storage

Obviously, such a group needs storage. Due to the facts:

- repeatedly executable
  - no single target for a future object available
- storable without background knowledge

each task should be of type `std::packaged_task< void() >`.

Consequently, each group would be stored as a `std::vector< std::packaged_task< void() > >`.

And by selecting a `std::size_t` as `ID`, hashing functions are native, so a map of groups does indeed make sense:
→ `std::map< std::size_t, std::vector< std::packaged_task< void() > > >`

---

### planning: pool design

Our task groups are already "in place".  How about thread-safe execution management?

A `std::queue` or a `std::deque` would be best.  (Well, a queue in this case - a deque if you had to check for target core efficiencies …)

Now - let's see about those 2 cases: if we're returning a future, there is only one way to "store" the task:
`std::move_only_function< void() >`

Why? Easy! The `std::future` return option needs to bind and return the future already when being enqueued - how else should userspace get its `std::future`?

The other path is much easier. Instancing a `std::move_only_function` from a `std::packaged_task` is directly supported.

#### pool design - storage

- `std::vector< std::jthread >` - worker thread (consumer) instances …
- `std::stop_source` - used by all workers, for graceful shutdown during destruction
- `std::mutex` + `std::condition_variable` - communication between pool and consumers
- `std::queue< std::move_only_function< void() > >` - Pool of active tasks to be "pulled" by consumer threads.
- `std::map` [(see group storage)](#group-storage)
- some integers: `free`, `running` (those do not need extra protection, as they will be modified with the mutex locked, only)
  - maybe some metrics in case those can be collected easily …

---

---

[← back to overview](overview.md)
