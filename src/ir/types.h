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
    CMP,
    JUMP,
    JA,
    RET,
    PHI,
    // TODO: what a difference between U32_TO_U64 and CAST? Need to remove one of them
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

