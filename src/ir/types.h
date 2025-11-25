#pragma once

#include <cstdint>

enum class Type {
    VOID,
    BOOL,
    U32,
    S32,
    U64,
};

enum class Opcode {
    Constant,
    Argument,
    ADD,
    MUL,
    AND,
    SHL,
    CMP,
    JUMP,
    JA,
    RET,
    PHI,
    U32_TO_U64,
    CAST,
};

enum class ConditionCode {
    EQ,
    NE,
    LT,
    GT,
    LE,
    GE,
    UGT,
    ULE,
};