## JIT/AOT IR Project

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
