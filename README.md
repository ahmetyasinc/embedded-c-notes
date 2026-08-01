# Embedded C Notes

Ten small programs covering the C fundamentals that embedded work actually leans on —
pointers, bits, `volatile`, memory layout, and the rest.

Each file is a single, self-contained program that **runs** and prints what it is
demonstrating. Where a concept only shows up in the generated code or in a crash, the
notes say which command to run to see it. Nothing here is pseudocode.

Written while preparing for embedded systems and edge AI work. Comments explain *why*,
not *what*.

## Build and run

Every file is independent, with no dependencies beyond libc:

```bash
gcc -Wall -o 01-pointer-arithmetic 01-pointer-arithmetic.c && ./01-pointer-arithmetic
```

Tested with GCC on Linux.

## Contents

| File | Topic | What it demonstrates |
|---|---|---|
| `01-pointer-arithmetic.c` | Pointers | Arithmetic scales by the pointed-to type; pass-by-pointer; array decay; writing to a fixed address |
| `02-bit-manipulation.c` | Bit manipulation | Set / clear / toggle / test; why `=` is not "set"; multi-bit fields; two ways to count set bits |
| `03-volatile.c` | `volatile` | The optimiser deleting correct code, proved three ways |
| `04-static.c` | `static` | Persistence inside a function, internal linkage outside it, and the reentrancy trap |
| `05-const.c` | `const` | Which side of the `*` the promise applies to; why a const table costs no RAM |
| `06-struct-padding.c` | Struct padding | The same members in two orders giving two different sizes |
| `07-endianness.c` | Endianness | Why shifts are portable and `memcpy` is not |
| `08-union.c` | Unions | Reinterpreting bits where a cast would convert the value; tagged unions |
| `09-memory-map.c` | Memory map | `.text` / `.rodata` / `.data` / `.bss` / heap / stack, and what each costs |
| `10-function-pointers.c` | Function pointers | Callbacks, dispatch tables, state machines, and the interrupt vector table |

## A few of the runnable proofs

**File 03 — the compiler deletes a loop.** A signal handler stands in for an interrupt
handler. Built three ways: `-O0` exits, `-O2` hangs forever, `-O2 -DUSE_VOLATILE` exits.
The assembly shows why — without `volatile` the read is hoisted out and the loop body
becomes a jump to itself. In the same file, three writes to a plain global vanish from
the generated code entirely, while three writes to a `volatile` one all survive in order.

```bash
gcc -Wall -O2 -o 03-volatile 03-volatile.c && ./03-volatile   # hangs; Ctrl+C
gcc -O2 -S -o - 03-volatile.c                                 # see why
```

**File 06 — `memcmp` reports a difference between two structs whose every member is
equal.** It compared the padding bytes, and nothing ever wrote to those.

**File 08 — a cast gives `1065353216.0` where a union gives `1.0`,** from the same four
bytes. A cast converts the value; a union reinterprets the bits. This is the difference
that matters when a float arrives over a bus.

**File 04 — two calls to the same function inside one `printf` print the same text,**
because both returned the same static buffer. Which call ran first is not specified by
the language.

## Recurring theme

Most of these topics are one question in different clothes: **who decides the layout —
you, or the toolchain?** Padding, byte order, bit-field packing and section placement are
all decided by the compiler and the target unless you take the decision yourself. Code
that leaves the chip — over CAN, UART, or into a file — has to state its format
explicitly, with masks and shifts. Code that stays inside can let the compiler choose.
