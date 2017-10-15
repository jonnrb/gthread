# gthread
*green threads for C*

## Overview

For the Rutgers undergraduate Operating Systems class, we were assigned the
task of building a user-space threading library that implements the POSIX
threads API (pthread). Our solution, gthread accomplishes that task using
abstractions on the base platform and architecture.

## Context Switch

`ucontext.h` is quite deprecated (gone from POSIX '08) and is very _very_ slow.
We implemented our own context switch. It piggybacks off of the function
calling convention on x86-64 and is *10x* faster than the `swapcontext()`
context switch in `ucontext.h`.

To run our microbenchmark:
```bash
make bin-test/arch/switch_to_test
bin-test/arch/switch_to_test
```
