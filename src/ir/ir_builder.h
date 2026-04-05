#pragma once

#include "ir/types.h"
#include <vector>

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
class MoveInst;
class LoadInst;
class StoreInst;
class CallStaticInst;

class IRBuilder {
  public:
    explicit IRBuilder(Graph *graph);

    void SetInsertPoint(BasicBlock *bb);
    void SetInsertPoint(Instruction *inst);

    ConstantInst *CreateConstant(Type type, uint64_t value);

    BinaryInst *CreateAdd(Instruction *lhs, Instruction *rhs);
    BinaryInst *CreateMul(Instruction *lhs, Instruction *rhs);
    BinaryInst *CreateAnd(Instruction *lhs, Instruction *rhs);
    BinaryInst *CreateShl(Instruction *lhs, Instruction *rhs);

    CompareInst *CreateCmp(ConditionCode cc, Instruction *lhs, Instruction *rhs);

    JumpInst *CreateJump(BasicBlock *target);
    BranchInst *CreateBranch(Instruction *cond, BasicBlock *true_bb, BasicBlock *false_bb);
    ReturnInst *CreateRet(Instruction *value);

    ArgumentInst *CreateArgument(Type type);
    CastInst *CreateCast(Type to_type, Instruction *from);
    PhiInst *CreatePhi(Type type);

    MoveInst *CreateMove(Type type, Instruction *from);
    LoadInst *CreateLoad(Type type, Instruction *from);
    StoreInst *CreateStore(Type type, Instruction *value, Instruction *to);

    CallStaticInst *CreateCallStatic(Graph *callee, const std::vector<Instruction *> &args);

  private:
    template <typename InstType, typename... Args> InstType *CreateInstruction(Args &&...args);

    Graph *graph_ = nullptr;
    BasicBlock *insert_bb_ = nullptr;
    Instruction *insert_before_ = nullptr;
};
