# JIT/AOT Compiler Backend

This repository contains a pedagogical JIT/AOT compiler backend implementation, developed as part of a compiler engineering course at the Moscow Institute of Physics and Technology (MIPT). The project focuses on the implementation of an SSA-based Intermediate Representation (IR), various static analyses, and advanced optimizations.

## 🚀 Tech Stack

- **Language**: C++20
- **Build System**: CMake (3.20+)
- **Testing Framework**: [GoogleTest](https://github.com/google/googletest)
- **Tooling**: `clang-format` for code style enforcement

## 🛠 Key Features & Components

### Intermediate Representation (IR)
- **SSA-based Graph**: A control-flow graph (CFG) where `Graph` containers hold `BasicBlock`s, which in turn contain `Instruction`s.
- **Typed Instructions**: Support for multiple data types (U32, U64, Bool, etc.).
- **User-Def Chains**: Built-in support for tracking instruction usages.
- **Explicit Phi Nodes**: IR correctly represents merge points in SSA form.

### Static Analysis
- **Graph Analysis**: Dominance frontier, Dominator Tree, and DFS traversals (RPO).
- **Loop Analysis**: Identification of loops, back-edges, and loop nesting structure.
- **Liveness Analysis**: Computation of `LiveInterval`s for all virtual registers.
- **Bounds Analysis**: Analysis of loop induction variables and array access bounds.

### Optimizations
- **Register Allocation**: Implementation of **Linear Scan on SSA** (based on the Wimmer & Mössenböck algorithm). Handles spilling and phi resolution via move insertion.
- **Inlining**: Basic static function inlining.
- **Checks Elimination**: Redundant `NULL_CHECK` and `BOUNDS_CHECK` removal using dominance information and loop bounds.
- **Peephole Optimization**: Algebraic simplifications and constant folding.

## 📁 Project Structure

The project was developed incrementally through a series of tasks:
- **Task 1-2**: IR construction (Graph, Blocks, Instructions) and Graph Analysis (Dominance).
- **Task 3-4**: Peephole optimizations and Loop Analysis.
- **Task 5-6**: Liveness Analysis and Linear Scan Register Allocation.
- **Task 7-8**: Inlining and Redundant Checks Elimination.

## 🔧 Installation & Build

### Prerequisites
- A C++20 compatible compiler (GCC 10+, Clang 12+)
- CMake 3.20 or higher
- Ninja (optional, but recommended)

### Step 1: Clone and Initialize Submodules
```bash
git clone --recursive https://github.com/your-repo/jit-aot.git
# Or if already cloned:
git submodule update --init --recursive
```

### Step 2: Build the Project
```bash
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

## 🧪 Usage & Testing

### Running Tests
All components are covered by unit tests. You can run them using `ctest`:
```bash
cd build
ctest
```

### Example: Factorial IR Dump
To see the IR representation of a factorial function:
```bash
cd build
./dump_factorial_ir
```

**Example Output:**
```
Function Arguments:
  i0.u32 Argument -> (i3)

BB0:
  Preds: -
  i1.u64 Constant 1 -> (i5, i10)
  i2.u64 Constant 2 -> (i6)
  i3.u64 Cast (i0) -> (i7)
  jump BB1
  Succs: BB1

BB1:
  Preds: BB0, BB2
  i6p.u64 Phi (i2, i10) -> (i10, i9, i7)
  i5p.u64 Phi (i1, i9) -> (i12, i9)
  i7.bool Cmp(ule) (i6, i3) -> (i8)
  branch i7 to BB2, BB3
  Succs: BB2, BB3

BB2:
  Preds: BB1
  i9.u64 Mul (i5, i6) -> (i5)
  i10.u64 Add (i6, i1) -> (i6)
  jump BB1
  Succs: BB1

BB3:
  Preds: BB1
  ret i5
  Succs: -
```
