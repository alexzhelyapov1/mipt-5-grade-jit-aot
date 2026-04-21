# JIT/AOT IR Project

# 8 task
- src/ir/analysis/bounds_analysis.h
- src/ir/analysis/bounds_analysis.cpp
- src/ir/opt/checks_elimination.h
- src/ir/opt/checks_elimination.cpp

Tests:
- tests/checks_elimination_test.cpp

# 7 task
Source files:
- src/ir/opt/inliner.h
- src/ir/opt/inliner.cpp

Tests:
- tests/inliner_test.cpp

# 6 task
Source files:
- `src/ir/opt/register_allocator.cpp`
- `src/ir/opt/register_allocator.h`

Tests (run with `ctest`):
- `tests/register_allocator_test.cpp`

Note: The penultimate commit

# 5 task
Source files:
- `src/ir/analysis/linear_order.h`
- `src/ir/analysis/linear_order.cpp`
- `src/ir/analysis/live_interval.cpp`
- `src/ir/analysis/live_interval.h`
- `src/ir/analysis/liveness_analyzer.cpp`
- `src/ir/analysis/liveness_analyzer.h`

Tests (run with `ctest`):
- `tests/linear_order_test.cpp`
- `tests/liveness_analysis_test.cpp`

# 3 task
See last commit in main branch.


# 2 task
- Graph analyzer implementation see in `src/ir/analysis`.
- Graph analyzer tests see in `tests/graph_analyzer_test.cpp`.


# 1 task

(JIT/AOT): граф, базовые блоки, инструкции, билдер и дамп в текстовый вид.

### Инициализация:
```bash
git submodule update --init --recursive
```

### Сборка
```bash
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

### Тесты
```bash
cd build
ctest
```

### Дамп IR факториала
```bash
cd build
./dump_factorial_ir
```

Результат:

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
