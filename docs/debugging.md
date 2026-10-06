# cpu_info Documentation - debugging

[→ back to overview](overview.md)

---

## Debug information parser

There are multiple types of debug support files a cpp programmer can create.

- `natvis` files for Visual Studio (also partially working for VSCode/Linux!)
- `gdb pretty printers`
- `lldb formats`
  - simple lldb commands to describe type format strings (lldb script)
  - Python-handled implementation of the types to visualize

Due to the fact this project was developed mostly with `Visual Studio Insiders` on Windows 11, and `VS Code Insiders` on Linux (Fedora 44), the 1st choice were `natvis` files.

*Testing around* with LLDB showed, that for a few simple types a simple lldb script is sufficient.

Further *"testing around"* was omitted due to its inherent energy consumption - e.g. I found it to be over-engineering.

---

## natvis file

The `cpu_info.natvis` file does a great job debugging under windows, as it uses the Microsoft Code for element parsing and cpu_info code for displaying.

Sadly, I did not find the patience to also look for the glibc-natvis-implementation, which I suspect to be existing somehow, but couldn't find in due time.

Some tries were taken to produce valid Linux and Windows output where this works out fine. For example `cpu_set` and `cpuid_result` get displayed nicely on both systems.

## .lldb and .py

I wanted to check out, what it costs to produce better usable output using lldb.  The result is inside `cpu_info.lldb`.

As already stated, no more efforts were taken, thus the `cpu_info-lldb.py` file is empty.

---

[← back to overview](overview.md)
