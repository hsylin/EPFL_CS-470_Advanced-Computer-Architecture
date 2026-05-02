# Design Notes

## 0. Assumptions and Limitations

The implementation follows the simplified assumptions of the homework:

- The input contains at most one loop.
- Nested loops are not supported.
- The program is split into `BB0`, `BB1`, and `BB2`.
- Only true register dependencies are tracked explicitly.
- Anti-dependencies and output dependencies are handled by register renaming.
- Memory dependencies are not modeled beyond register operands used by load and store instructions.
- The implementation assumes enough registers are available, except for explicit range checks on static and rotating register files.
- Register spilling is not implemented.
- For programs without a loop, the same straight-line schedule is written to both output files.


## 1. Overview

This project implements a compiler-style scheduler for the VLIW470 processor. Given an input program in JSON format, the program produces two scheduled VLIW outputs:

1. `loop.json`: a schedule using the normal `loop` instruction.
2. `looppip.json`: a software-pipelined schedule using the `loop.pip` instruction, rotating registers, rotating predicates, and epilogue control.

The implementation is organized as a shared frontend and middle-end, followed by two backend paths:

```text
Input program
    |
    v
Shared frontend
    - JSON loading
    - Instruction parsing
    - Basic-block splitting
    |
    v
Shared middle-end
    - Dependency analysis
    |
    +--> Normal loop backend
    |       - ASAP scheduling
    |       - Normal register allocation
    |       - Interloop-fixing mov insertion
    |       - loop.json emission
    |
    +--> loop.pip backend
            - II_res computation
            - Modulo / software-pipelined scheduling
            - Rotating register allocation
            - Predicate and EC initialization
            - Compact loop-body generation
            - looppip.json emission
```

The main design goal is to separate concerns clearly. The frontend understands the input program, the middle-end computes dependency information, and the backends decide how to schedule and allocate registers for the two different loop mechanisms.

---

## 2. Instruction Representation

`instruction.hpp` defines the central instruction data model used throughout the project. This is especially important in HW2 because the instruction set is richer than in HW1: it contains arithmetic instructions, memory instructions, `mov`, `loop`, `loop.pip`, predicate registers, special registers, and memory operands.

This common representation allows later compiler phases to treat different instruction formats uniformly.

## 3. Shared Frontend

The frontend converts the raw JSON input program into a structured internal representation.

### Basic-Block Splitting

`basic_block.cpp` divides the program into three blocks:

- `BB0`: initialization code before the loop body.
- `BB1`: loop body, including the loop instruction.
- `BB2`: finalization code after the loop.

This classification is essential because the same register dependence has different meanings depending on where the producer and consumer are located.

For example:

```text
BB0 writes x2: initial value before the loop
BB1 writes x2: next-iteration value
BB2 reads x2: final value after the loop
```

Therefore, splitting the program into `BB0`, `BB1`, and `BB2` is not only a structural convenience. It determines the execution-time meaning of each dependency and directly affects both 1. the scheduling constraints  and 2. the register-allocation strategy.

For straight-line programs without a loop, the implementation treats the whole program as `BB0` and emits the same schedule for both output files.

---

## 4. Shared Middle-End: Dependency Analysis

The middle-end is responsible for program analysis and schedule preparation. In this project, its main role is to compute true data-dependency information, i.e., read-after-write dependencies, which the backend uses for scheduling, register allocation, and loop-specific code generation.


The dependency analysis classifies dependencies into four categories:

1. Local dependencies
2. Interloop dependencies
3. Loop-invariant dependencies
4. Post-loop dependencies

Anti-dependencies and output dependencies are not explicitly tracked because they are handled by register renaming during register allocation.




---

## 5. `looppip.json` Backend

The `loop.pip` backend implements software pipelining. Unlike the normal backend, it does not unroll all dynamic loop iterations. Instead, it schedules one compact loop kernel and relies on hardware support from `loop.pip` to overlap iterations at runtime.

Conceptually, software pipelining behaves as if multiple loop iterations are executing at the same time:

```text
Iteration 0: stage 0, stage 1, stage 2, ...
Iteration 1:          stage 0, stage 1, stage 2, ...
Iteration 2:                   stage 0, stage 1, stage 2, ...
```

However, the emitted program is compact. It contains only a fixed-size loop body of length `II`, not a fully unrolled execution trace. This keeps the generated program small and independent of the dynamic loop count.


### Initiation Interval (`II`)

A larger `II` gives each loop iteration more available processor resources. For example, if `II = 4`, an iteration can use four issue cycles of resources before the next iteration starts. However, a larger `II` also reduces the overlap between iterations, making the loop less pipelined. Therefore, the scheduler aims to minimize `II` while still satisfying resource constraints and loop-carried dependencies, such as accumulator dependencies. Since different `II` values can lead to different schedules, finding a valid `II` is an iterative process.

### End-to-End Data Flow

The complete internal data flow is:

```text
program + dependency_table
    |
    v
schedule_looppip()
    |
    v
looppip_schedule_result_t / scheduled_program_t
    |
    v
allocate_rotating_registers()
    |
    v
allocated schedule_t
    |
    v
prepare_looppip_schedule()
    |
    v
final compact schedule_t
    |
    v
write_schedule(looppip.json)
```

---

## 6. Summary

The implementation is structured as a compiler pipeline for VLIW scheduling. The shared frontend parses the input into a rich instruction representation, and the shared middle-end classifies dependencies according to their execution-time meaning. The normal backend emits a conventional `loop` schedule with register renaming and interloop-fixing moves. The `loop.pip` backend performs modulo scheduling, rotating register allocation, predicate insertion, and compact loop-body generation.

The main design principle is to keep scheduling, dependency analysis, register allocation, and final JSON emission separate. This makes the implementation easier to reason about and allows the two output paths, `loop.json` and `looppip.json`, to share as much infrastructure as possible while still handling their different execution models correctly.