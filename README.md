# cpu_info 

A small class for gathering CPU information, that most OS do not provide …

---

## Motivation

When designing and creating multithreaded applications, you run into the same issues over and over.

One issue might be: "spawning more worker threads than PHYSICAL cpu cores available introduces slow downs".

This actually is the most asked question in multithreading, at least of those questions I had to answer.

For a decade, simple CPUid was sufficient on desktop systems.  The next decade went with a 3-level-option.  Nowadays, it is indeed a good thing vendors like intel provide example code to find those answers deterministically.

---

## How to use

There are probably many options how to use the code at hand. Most obvious are:
- use FetchContent and link the library
- import cpu_info.h and cpu_info.cpp into your build tree

> NOTE: c++20 is required for compilation due to the use of `std::format` and `std::vformat`
