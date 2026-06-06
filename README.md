# Advanced Computer Architecture

This repository contains the implementations of the practical assignments for EPFL’s **Advanced Computer Architecture** course (2026).
---

## 1. A Cycle-By-Cycle Simulator of an Out-of-Order Processor

- **Objective**: Build a cycle-exact simulator of a simple out-of-order processor, **OoO470**, that executes a minimal subset of the RISC-V arithmetic instruction set and dumps the processor’s internal microarchitectural state at the end of every cycle.
- **Assignment Details**: [Homework 1 - A Cycle-By-Cycle Simulator of an Out-of-Order Processor](./HW1/documentation/homework1.pdf)
- **Design Notes**: [Design Notes](./HW1/documentation/design_note.md)


## 2. A Cycle-By-Cycle Scheduler for a VLIW Processor

- **Objective**: Build a scheduler for a simple VLIW processor, **VLIW470**, that statically analyzes instruction dependencies and extracts instruction-level parallelism. The scheduler produces two VLIW outputs:
  1. `loop.json`: a schedule using the normal `loop` instruction.
  2. `looppip.json`: a software-pipelined schedule using the `loop.pip` instruction, rotating registers, rotating predicates, and epilogue control.
- **Assignment Details**: [Homework 2 - A Cycle-By-Cycle Scheduler for a VLIW Processor](./HW2/documentation/homework2.pdf)
- **Design Notes**: [Design Notes](./HW2/documentation/design_note.md)


## 3. High-Level Synthesis Optimization

- **Objective**: Analyze and optimize five High-Level Synthesis (HLS) kernels under fixed operation latencies and single-port BRAM constraints. The assignment explores loop pipelining, array partitioning, loop-carried dependency analysis, scalar-register rewrites, memory-port constraints, and the performance trade-offs between static and dynamic HLS.
- **Assignment Details**: [Homework 3 - High-Level Synthesis](./HW3/documentation/homework3.pdf)
- **Submission**: [Homework 3 Submission](./HW3/submission.pdf)

## 4. Spectre Attack

- **Objective**: Implement a Spectre-style cache side-channel attack against a bounds-checked victim function. The attack trains the branch predictor, extends the speculation window by delaying branch resolution, and recovers secret bytes by measuring cache-access timing through a Flush+Reload side channel.
- **Assignment Details**: [Homework 4 - Spectre](./HW4/documentation/homework4.pdf)
- **Report**: [Homework 4 Report](./HW4/report.pdf)
- **Source Code**: [Spectre Attack Implementation](./HW4/spectre.c)


