# gthread

_green threads for C_

## Overview

For the Rutgers undergraduate Operating Systems class, we were assigned the
task of building a user-space threading library that implements the POSIX
threads API (pthread). Our solution, gthread accomplishes that task by using
abstractions on the base platform and architecture to build a fair scheduler.

Our implementation uses an extremely fast context switch and implements thread
local storage on both macOS and Linux for x86-64 systems. Our project could be
improved by adding additional concurrency primitives that would enable
cooperative multitasking between tasks (a practical application of user-space
threads in modern software systems).

## Modularity

- `arch`: simple wrappers for x86-64 things like atomics and our context switch

- `platform`: platform things (some of our group members use macOS, and things
  differ from Linux in a few ways)

- `concur`: concurrency primitives, i.e., a mutex

- `sched`: the scheduler (the interesting bit)

## Context Switch

`ucontext.h` is quite deprecated (gone from POSIX '08) and is very _very_ slow.
We implemented our own context switch. It piggybacks off of the function
calling convention on x86-64 and is _10x_ faster than the `swapcontext()`
context switch in `ucontext.h`.

To run our microbenchmark:

```bash
make bin-test/arch/switch_to_test
bin-test/arch/switch_to_test
```

## Fair Scheduler

Our scheduler attempts to be fair. We track the virtual runtime of each task
and schedule the tasks with the minimum virtual runtime first. Processes that
age for longer (have to be preempted often) get run less frequently.

Because our priorities are updated continuously, we needed a fast priority
queue that allowed fast inserts anywhere. We used a tree structure that gives
us *O(log n)* average case performance (located in `util/rb.{c,h}`).

## Test Suite

Our project is broken up into modules that have independent unit tests. Our
test suite can be run with `make test`, which runs all the tests and checks
that they run successfully.

The benchmark provided with the project description can optionally be run with
`make bench`.
