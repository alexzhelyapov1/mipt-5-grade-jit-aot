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
class JumpInst;
class BranchInst;
class ReturnInst;


class IRBuilder {
public:
    explicit IRBuilder(Graph* graph);

    void SetInsertPoint(BasicBlock* bb);

    ConstantInst* CreateConstant(Type type, uint64_t value);

    BinaryInst* CreateAdd(Instruction* lhs, Instruction* rhs);
    BinaryInst* CreateMul(Instruction* lhs, Instruction* rhs);
    BinaryInst* CreateAnd(Instruction* lhs, Instruction* rhs);
    BinaryInst* CreateShl(Instruction* lhs, Instruction* rhs);

    CompareInst* CreateCmp(ConditionCode cc, Instruction* lhs, Instruction* rhs);

    JumpInst* CreateJump(BasicBlock* target);
    BranchInst* CreateBranch(Instruction* cond, BasicBlock* true_bb, BasicBlock* false_bb);
    ReturnInst* CreateRet(Instruction* value);

    ArgumentInst* CreateArgument(Type type);
    CastInst* CreateCast(Type to_type, Instruction* from);
    PhiInst* CreatePhi(Type type);

private:
    template<typename InstType, typename... Args>
    InstType* CreateInstruction(Args&&... args);

    Graph* graph_ = nullptr;
    BasicBlock* insert_point_ = nullptr;
};


