# Why EmberSim

## Problem

Embedded firmware validation depends on physical hardware.

## Existing workflow

```
Write firmware
      │
Wait for board
      │
Flash + debugger
      │
Reproduce issue manually
      │
Inspect registers
      │
Fix + repeat
```

## EmberSim workflow

```
Commit firmware
      │
CI executes natively
      │
Deterministic trace
      │
Failure diagnosed from trace
      │
Hardware optional
```

## Success metric

One hardware-dependent regression test becomes a deterministic CI test.

---

*This page is a decision filter. Every feature should support this story. If a proposed feature doesn't move the project closer to this workflow, question whether it belongs.*
