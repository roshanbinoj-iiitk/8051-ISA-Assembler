# ISA Assembler and Emulator (8051-Inspired, C++)

Minimal but usable C++ demo that assembles a tiny 8051-style assembly subset into machine code and executes it in a cycle-counted emulator.

**Project Status:** In Progress

## Overview

This repository contains two core modules:

1. **Assembler (`Assembler`)**
   - Parses `.asm` source.
   - Encodes instructions into bytes.
   - Supports labels with a two-pass approach.
2. **CPU Emulator (`CPU`)**
   - Models memory + registers (`A`, `B`, `R0..R7`, `PC`).
   - Executes a fetch-decode-execute loop.
   - Tracks total cycles and supports trace mode.

The demo is designed to be clone-build-run friendly in under 5 minutes.

## MVP Feature Set

- End-to-end flow: `.asm` -> `.bin` -> emulator execution.
- Instruction support:
  - `MOV A, #imm8`
  - `ADD A, Rn` (`R0..R7`)
  - `SJMP rel8` / `SJMP label`
  - `NOP`
  - `HLT` (custom stop instruction)
- Emulator safety checks for malformed instruction reads and out-of-range jumps.
- Optional per-step trace output (`--trace`).
- Simple CLI commands:
  - `assemble input.asm output.bin`
  - `run program.bin`
  - `asmrun input.asm`

## Architecture

### CPU Module

- File pair: `include/cpu.hpp` + `src/cpu.cpp`
- Responsibilities:
  - Maintain machine state.
  - Execute one instruction in `step()`.
  - Run loop in `run(maxSteps)`.
  - Keep instruction timing via `cycles_`.

### Assembler Module

- File pair: `include/assembler.hpp` + `src/assembler.cpp`
- Responsibilities:
  - Parse assembly lines and strip comments.
  - First pass: collect labels + instruction addresses.
  - Second pass: encode bytes and resolve `SJMP` offsets.
  - Emit byte vector and write `.bin` output.

### Fetch-Decode-Execute (Implemented)

Inside `CPU::step()`:

1. **Fetch** opcode at current `PC`.
2. **Decode** instruction via opcode handling.
3. **Execute** instruction semantics.
4. Update `PC` and `cycles_`.

This design keeps instruction behavior explicit and easy to extend.

## Instruction Encoding Reference

Source of truth is `spec/opcodes.csv`.

| Mnemonic     | Encoding     | Bytes | Cycles |
| ------------ | ------------ | ----- | ------ |
| `NOP`        | `0x00`       | 1     | 1      |
| `MOV A,#imm` | `0x74 imm8`  | 2     | 1      |
| `ADD A,R0`   | `0x28`       | 1     | 1      |
| `ADD A,R1`   | `0x29`       | 1     | 1      |
| `...`        | `0x2A..0x2F` | 1     | 1      |
| `SJMP rel8`  | `0x80 rel8`  | 2     | 2      |
| `HLT`        | `0xFF`       | 1     | 1      |

## Project Structure

```text
isa-assembler/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── assembler.hpp
│   └── cpu.hpp
├── src/
│   ├── assembler.cpp
│   ├── cpu.cpp
│   └── main.cpp
├── examples/
│   ├── add.asm
│   └── branch.asm
└── spec/
    └── opcodes.csv
```

## Build (CMake)

### Requirements

- CMake >= 3.16
- C++17 compiler
  - `g++` (recommended >= 11)
  - `clang++` (recommended >= 14)

### Commands

```bash
cmake -S . -B build
cmake --build build
```

Binary produced: `build/isa_tool`

## CLI Usage

```bash
# Assemble .asm into .bin
./build/isa_tool assemble <input.asm> <output.bin>

# Run a binary program
./build/isa_tool run <program.bin> [--trace] [--r0 <0..255>]

# Assemble and run in one command
./build/isa_tool asmrun <input.asm> [--trace] [--r0 <0..255>]
```

`--r0` is useful for quick demos where `ADD A, R0` is used.

## Example Program

Example file: `examples/add.asm`

```asm
MOV A, #5
ADD A, R0
HLT
```

Run it with `R0 = 3`:

```bash
./build/isa_tool asmrun examples/add.asm --r0 3
```

Expected result includes:

```text
Execution finished.
Final A = 8
```

## Quick End-to-End Demo

```bash
# 1) Build
cmake -S . -B build
cmake --build build

# 2) Assemble
./build/isa_tool assemble examples/add.asm examples/add.bin

# 3) Run
./build/isa_tool run examples/add.bin --r0 3
```

## Minimal Robustness Notes

- Invalid opcodes fail with a clear error message.
- `SJMP` target bounds are checked.
- Multi-byte instruction fetches are validated before reads.
- Assembly parse errors include source line context.

## Current Roadmap Reference

The original plan remains valid and is now partially implemented:

1. CPU class + `step()` and `run()`.
2. Core instructions (`NOP`, `MOV`, `ADD`, `SJMP`, `HLT`).
3. Trace mode for execution visibility.
4. Two-pass assembler with labels.
5. Unified CLI workflow.
6. Next: tests, CI, and broader instruction coverage.

## Future Improvements

- Add unit tests and golden integration tests.
- Add GitHub Actions CI (`build + test`).
- Extend ISA with a few more instructions (`INC`, `DEC`, compare/branch).
- Add optional Intel HEX output.
- Add richer debugger features (breakpoints, memory dump).

## Resume-Ready Summary

- Developed a cycle-counted 8051-inspired ISA emulator in C++ with explicit fetch-decode-execute semantics.
- Built a two-pass assembler that resolves labels and emits executable machine code for end-to-end program execution.
