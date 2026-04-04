# Design Notes

This CPU simulator adopts a **state-centric propagate–latch design**.

At cycle `N`, the processor is considered to be in one precise and fully defined **state**. Each unit reads this current state and contributes to determining what the processor state should be at cycle `N+1`. In this sense, the simulator is fundamentally **state-driven**: processor behavior is modeled as a transition from the current state to the next state.

Conceptually, the model can be summarized as:

```text
current state + combinational logic → next state
```

In this framework, **registers and memories** are responsible for storing the processor’s architectural and microarchitectural state. By contrast, each **unit** is treated as **combinational logic**: it does not maintain persistent state of its own, but instead reads the current state and computes part of the next state.

The simulator therefore separates execution into two phases:

## Propagate Phase

This phase models the behavior of **combinational logic**. Each unit examines the current state and computes updates that contribute to the next state.

## Latch Phase

This phase models the effect of a **clock edge**. The computed next-state values are committed, and the processor advances from cycle `N` to cycle `N+1`.

In short, the simulator does not model the CPU as a physically moving pipeline evolving in real time. Instead, it models the processor as a sequence of well-defined state transitions, where **propagate** computes the next state and **latch** makes that state official for the following cycle.

# Code Organization

In this implementation:

- the **state** is defined in the `data_structures` folder
- the **combinational logic** is implemented in the `microarchitecture` folder

This reflects the conceptual separation between stored processor state and the logic that transforms that state.

# How Time and Order Are Simulated

The simulator does not literally model hardware as a physical pipeline advancing one stage at a time in real time. Instead, it uses the execution order of the program to simulate which effects are allowed to happen within the same cycle.

That is:

- **same-cycle behavior** is determined by **code order**
- **cross-cycle behavior** is determined by the separation between **current state** and **next state**

## Same-Cycle Order

At the top level, `main.cpp` already defines the execution order of the stages. Therefore, each `propagate()` function does not need to manage the global pipeline ordering by itself.

The simulator uses the following conceptual containers:

- `curr`: the official processor state at the start of the cycle
- `view`: working copies of same-cycle-visible queues, such as the Active List, Integer Queue, and Free List
- `next`: the final next-state container that will be latched at the end of the cycle
- `ctx`: transient per-cycle information, such as forwarding results, exception signals, and control requests

Because the top-level execution order is already fixed in `main.cpp`, each unit only needs to focus on its own combinational behavior under that ordering.

## Different-Cycle Order

Different-cycle behavior is modeled through the separation between **current state** and **next state**. A typical pattern is:

```cpp
State curr = state;
State next = curr;
```

Here, `curr` represents the official state for the current cycle, while `next` starts as a copy of `curr` and is then modified during propagation. The updates made to `next` do not become visible as official processor state until the latch step.

This separation is what allows the simulator to represent the hardware idea that:

- the processor begins a cycle with one stable state
- combinational logic computes updates based on that state
- those updates only become architecturally visible at the clock edge

# Summary

Overall, this simulator is best understood as a **state-transition model** rather than a physical timing model of a pipeline. The key ideas are:

1. The processor starts each cycle in one well-defined **current state**.
2. Units act as **combinational logic**, reading that state and contributing to the **next state**.
3. The top-level code order determines what is visible within the same cycle.
4. The separation between `curr` and `next` determines what is deferred to the following cycle.
5. The latch step commits the computed next state and advances the processor to the next cycle.
