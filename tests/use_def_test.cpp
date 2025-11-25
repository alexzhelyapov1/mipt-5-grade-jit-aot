#include <gtest/gtest.h>
#include "ir/ir.h"

TEST(UseDefTest, BinaryAddRegistersUses) {
    Graph graph;
    IRBuilder builder(&graph);
    auto* basic_block = graph.CreateBasicBlock();
    builder.SetInsertPoint(basic_block);

    auto* arg1 = builder.CreateArgument(Type::U32);
    auto* arg2 = builder.CreateArgument(Type::U32);
    Instruction* add_inst = builder.CreateAdd(arg1, arg2);

    auto* u1 = arg1->GetFirstUser();
    ASSERT_NE(u1, nullptr);
    EXPECT_EQ(u1->GetUserInstruction(), add_inst);

    auto* u2 = arg2->GetFirstUser();
    ASSERT_NE(u2, nullptr);
    EXPECT_EQ(u2->GetUserInstruction(), add_inst);
}

TEST(UseDefTest, PhiAddIncomingRegistersUses) {
    Graph graph;
    IRBuilder builder(&graph);
    auto* entry_bb = graph.CreateBasicBlock();
    auto* loop_bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(entry_bb);

    builder.CreateJump(loop_bb);

    builder.SetInsertPoint(loop_bb);
    auto* phi = builder.CreatePhi(Type::U32);

    auto* const_0 = builder.CreateConstant(Type::U32, 0);
    phi->AddIncoming(const_0, entry_bb);

    auto* use_on_c0 = const_0->GetFirstUser();
    ASSERT_NE(use_on_c0, nullptr);
    EXPECT_EQ(use_on_c0->GetUserInstruction(), phi);
}

