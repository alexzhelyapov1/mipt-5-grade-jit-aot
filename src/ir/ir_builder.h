#pragma once

#include "ir/types.h"

class Graph;
class BasicBlock;
class Instruction;
class ConstantInst;
class BinaryInst;
class CompareInst;
class ArgumentInst;
class CastInst;
class PhiInst;

class IRBuilder {
public:
    explicit IRBuilder(Graph* graph);

    void SetInsertPoint(BasicBlock* bb);

    ConstantInst* CreateConstant(Type type, uint64_t value);

    Instruction* CreateAdd(Instruction* lhs, Instruction* rhs);
    Instruction* CreateMul(Instruction* lhs, Instruction* rhs);
    Instruction* CreateAnd(Instruction* lhs, Instruction* rhs);
    Instruction* CreateShl(Instruction* lhs, Instruction* rhs);

    CompareInst* CreateCmp(ConditionCode cc, Instruction* lhs, Instruction* rhs);

    void CreateJump(BasicBlock* target);
    void CreateBranch(Instruction* cond, BasicBlock* true_bb, BasicBlock* false_bb);
    void CreateRet(Instruction* value);

    ArgumentInst* CreateArgument(Type type);
    CastInst* CreateCast(Type to_type, Instruction* from);
    PhiInst* CreatePhi(Type type);

private:
    template<typename InstType, typename... Args>
    InstType* CreateInstruction(Args&&... args);

    Graph* graph_ = nullptr;
    BasicBlock* insert_point_ = nullptr;
};
