#include "ir/ir.h"

void BuildFactorialGraph(Graph* graph) {
    IRBuilder builder(graph);
    ArgumentInst* arg_n = builder.CreateArgument(Type::U32);
    BasicBlock* entry_bb = graph->CreateBasicBlock();
    BasicBlock* loop_bb = graph->CreateBasicBlock();
    BasicBlock* body_bb = graph->CreateBasicBlock();
    BasicBlock* exit_bb = graph->CreateBasicBlock();

    builder.SetInsertPoint(entry_bb);
    ConstantInst* const_1_u64 = builder.CreateConstant(Type::U64, 1);
    ConstantInst* const_2_u64 = builder.CreateConstant(Type::U64, 2);
    CastInst* n_u64 = builder.CreateCast(Type::U64, arg_n);
    builder.CreateJump(loop_bb);

    builder.SetInsertPoint(loop_bb);
    PhiInst* res_phi = builder.CreatePhi(Type::U64);
    PhiInst* i_phi = builder.CreatePhi(Type::U64);
    CompareInst* cond = builder.CreateCmp(ConditionCode::ULE, i_phi, n_u64);
    builder.CreateBranch(cond, body_bb, exit_bb);

    builder.SetInsertPoint(body_bb);
    BinaryInst* next_res = builder.CreateMul(res_phi, i_phi);
    BinaryInst* next_i = builder.CreateAdd(i_phi, const_1_u64);
    builder.CreateJump(loop_bb);

    builder.SetInsertPoint(exit_bb);
    builder.CreateRet(res_phi);

    res_phi->AddIncoming(const_1_u64, entry_bb);
    res_phi->AddIncoming(next_res, body_bb);
    i_phi->AddIncoming(const_2_u64, entry_bb);
    i_phi->AddIncoming(next_i, body_bb);
}
