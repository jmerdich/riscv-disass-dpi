RISC-V Disassembler DPI Plugin
=============================

A small, self-contained library to be used to get readable instructions from
a binary RISC-V opcode when called from Verilog. This is a huge help when
debugging a RISC-V CPU you made!

Setup:
------

Varies by simulator, but in general you first compile this library (one file)
and then specify that library in your command to simulate. Check `tests/integration`
for some examples.


Usage:
-----

```systemverilog
    `include "rv_disass.svi"
    // ...
    string disass_output = rv_disass(inst);
    // ...
    
    // TODO: figure out the big gap in the DPI spec about who
    // is supposed to free a returned string. There are some
    // hacks in there now so it will 'just work' without an
    // explicit free on most simulators.
    // rv_free(disass_output);

```


FAQs:
-----

### Can I use this in (personal-project/big-commercial-project)?

Absolutely. Even your most stringent lawyers shouldn't raise any red flags for
you using this. I am annoyed enough that this *doesn't* exist that I don't even
require attribution.

This software (and all supporting content in this repo) is released under the
unlicense, which is functionally public domain.

Personal experience in big commercial projects has taught me to put this
front-and-center.

### What is this?

When I made a RISC-V CPU as a personal project, one of the biggest issues
I encountered was that when looking at wave dumps, I had to take a string
of bits and figure out what instruction that was. Needless to say, the
ISA manual's instruction tables are now very familiar.

This is a plugin written in C that can be used be a variety of Verilog
simulators using "DPI". It takes a RISC-V instruction in binary and returns
its' disassembled form (particularly useful for traces and wave dumps).

### So it can disassemble.... anything in RISC-V?

The goal is to disassemble any single instruction. Things that read multiple
instructions (including some pseudoinstructions!) are explicitly out of scope,
as are anything that requires reasoning about the program as a whole.

### How do I use this in (some-simulator)?

Check our examples. If it's not there, chances are, I don't know! Especially for
commercial sims. But file an issue anyways and if I find some info or someone
does find out, we can add it.

### What about this custom extension I'm working on?

Future me will try to provide an infrastructure to take info on
additional custom instructions that you can keep as a separate header
and compile flag-- you'll still be able to pull updates.

That said if it's public, PRs are welcome. This includes CSRs.

