#include "ir/ir.h"
#include "ir/ir_builder.h"
#include "ir/opt/inliner.h"
#include "ir/graph.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include <gtest/gtest.h>
#include <algorithm>

TEST(Inliner, SimpleInlining) {
    Graph callee;
    IRBuilder callee_builder(&callee);
    auto *bb = callee.CreateBasicBlock();
    callee.SetStartBlock(bb);
    callee_builder.SetInsertPoint(bb);
    auto *arg0 = callee_builder.CreateArgument(Type::U32);
    auto *c1 = callee_builder.CreateConstant(Type::U32, 1);
    auto *res = callee_builder.CreateAdd(arg0, c1);
    callee_builder.CreateRet(res);

    Graph caller;
    IRBuilder caller_builder(&caller);
    auto *b_start = caller.CreateBasicBlock();
    caller.SetStartBlock(b_start);
    caller_builder.SetInsertPoint(b_start);
    auto *c10 = caller_builder.CreateConstant(Type::U32, 10);
    auto *call = caller_builder.CreateCallStatic(&callee, {c10});
    call->SetReturnType(Type::U32);
    auto *c2 = caller_builder.CreateConstant(Type::U32, 2);
    auto *total = caller_builder.CreateAdd(call, c2);
    caller_builder.CreateRet(total);

    Inliner inliner(&caller);
    inliner.Run();

    bool call_found = false;
    for (auto &bb_it : caller.GetBlocks()) {
        for (auto *inst = bb_it.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::CALL_STATIC) call_found = true;
        }
    }
    EXPECT_FALSE(call_found);
}

TEST(Inliner, LectureExample) {
    // Callee graph: Start -> Body -> (Ret1 | Ret2)
    Graph callee;
    IRBuilder callee_builder(&callee);
    auto *c_start = callee.CreateBasicBlock();
    auto *c_body = callee.CreateBasicBlock();
    auto *c_ret1 = callee.CreateBasicBlock();
    auto *c_ret2 = callee.CreateBasicBlock();
    callee.SetStartBlock(c_start);

    callee_builder.SetInsertPoint(c_start);
    auto *p1 = callee_builder.CreateArgument(Type::U32);
    auto *p2 = callee_builder.CreateArgument(Type::U32);
    auto *c19 = callee_builder.CreateConstant(Type::U32, 1);
    auto *c20 = callee_builder.CreateConstant(Type::U32, 10);
    callee_builder.CreateJump(c_body);

    callee_builder.SetInsertPoint(c_body);
    auto *cmp = callee_builder.CreateCmp(ConditionCode::GT, p1, c19);
    callee_builder.CreateBranch(cmp, c_ret1, c_ret2);

    callee_builder.SetInsertPoint(c_ret1);
    auto *v15 = callee_builder.CreateAdd(p1, c19);
    callee_builder.CreateRet(v15);

    callee_builder.SetInsertPoint(c_ret2);
    auto *v17 = callee_builder.CreateAdd(p2, c20);
    callee_builder.CreateRet(v17);

    // Caller graph: B0 -> B1 (Contains the call to Callee)
    Graph caller;
    IRBuilder caller_builder(&caller);
    auto *b0 = caller.CreateBasicBlock();
    auto *b1 = caller.CreateBasicBlock();
    caller.SetStartBlock(b0);

    caller_builder.SetInsertPoint(b0);
    auto *v1 = caller_builder.CreateConstant(Type::U32, 1);
    auto *v2 = caller_builder.CreateConstant(Type::U32, 5);
    caller_builder.CreateJump(b1);

    caller_builder.SetInsertPoint(b1);
    auto *v3 = caller_builder.CreateAdd(v1, v2);
    auto *v4 = caller_builder.CreateMul(v1, v2);
    auto *call = caller_builder.CreateCallStatic(&callee, {v3, v4});
    call->SetReturnType(Type::U32);
    auto *use1 = caller_builder.CreateAdd(call, v1);
    caller_builder.CreateRet(use1);

    Inliner inliner(&caller);
    inliner.Run();

    // verify that CALL_STATIC instr was eliminated
    bool call_found = false;
    for (auto &bb_it : caller.GetBlocks()) {
        for (auto *inst = bb_it.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::CALL_STATIC) call_found = true;
        }
    }
    EXPECT_FALSE(call_found);

    // verify that constants from callee are hoisted to caller's entry block
    auto *start_bb = caller.GetStartBlock();
    Instruction *hoisted_c19 = nullptr;
    Instruction *hoisted_c20 = nullptr;
    for (auto *inst = start_bb->GetFirstInstruction(); inst; inst = inst->GetNext()) {
        if (inst->GetOpcode() == Opcode::Constant) {
            auto *c = static_cast<ConstantInst *>(inst);
            if (c->GetValue() == 1) hoisted_c19 = c;
            if (c->GetValue() == 10) hoisted_c20 = c;
        }
    }
    EXPECT_NE(hoisted_c19, nullptr);
    EXPECT_NE(hoisted_c20, nullptr);

    // verify that cloned instructions use correctly mapped parameters and hoisted constants
    int add_uses_found = 0;
    for (auto &bb_it : caller.GetBlocks()) {
        for (auto *inst = bb_it.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::ADD) {
                auto *add = static_cast<BinaryInst *>(inst);
                for (auto *input : add->GetInputs()) {
                    if (input == hoisted_c19 || input == hoisted_c20) {
                        add_uses_found++;
                    }
                }
            }
        }
    }
    EXPECT_GE(add_uses_found, 2);

    // phi node correctly merges multiple return values from the callee
    bool phi_found = false;
    for (auto &bb_it : caller.GetBlocks()) {
        for (auto *inst = bb_it.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::PHI) {
                phi_found = true;
                auto *phi = static_cast<PhiInst *>(inst);
                EXPECT_EQ(phi->GetInputs().size(), 2);
            }
        }
    }
    EXPECT_TRUE(phi_found);
}
