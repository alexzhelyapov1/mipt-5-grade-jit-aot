#include <gtest/gtest.h>
#include "ir/ir.h"

TEST(Optimization, AddConstantFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 10);
    auto* c2 = builder.CreateConstant(Type::U32, 32);
    auto* res = builder.CreateAdd(c1, c2);

    ASSERT_EQ(res->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(res)->GetValue(), 42);
}

TEST(Optimization, AddPeepholes) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* arg = builder.CreateArgument(Type::U32);
    auto* zero = builder.CreateConstant(Type::U32, 0);

    // X + 0 -> X
    auto* res1 = builder.CreateAdd(arg, zero);
    EXPECT_EQ(res1, arg);

    // 0 + X -> X
    auto* res2 = builder.CreateAdd(zero, arg);
    EXPECT_EQ(res2, arg);
}

TEST(Optimization, AndConstantFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 0b1100);
    auto* c2 = builder.CreateConstant(Type::U32, 0b1010);
    auto* res = builder.CreateAnd(c1, c2);

    ASSERT_EQ(res->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(res)->GetValue(), 0b1000);
}

TEST(Optimization, AndPeepholes) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* arg = builder.CreateArgument(Type::U32);
    auto* zero = builder.CreateConstant(Type::U32, 0);
    auto* all_ones = builder.CreateConstant(Type::U32, -1);

    // X & 0 -> 0
    auto* res1 = builder.CreateAnd(arg, zero);
    ASSERT_EQ(res1->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(res1)->GetValue(), 0);

    // X & X -> X
    auto* res2 = builder.CreateAnd(arg, arg);
    EXPECT_EQ(res2, arg);

    // X & -1 -> X
    auto* res3 = builder.CreateAnd(arg, all_ones);
    EXPECT_EQ(res3, arg);
}

TEST(Optimization, ShlConstantFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 3);
    auto* c2 = builder.CreateConstant(Type::U32, 2);
    auto* res = builder.CreateShl(c1, c2);

    ASSERT_EQ(res->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(res)->GetValue(), 12);
}

TEST(Optimization, ShlPeepholes) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* arg = builder.CreateArgument(Type::U32);
    auto* zero = builder.CreateConstant(Type::U32, 0);
    auto* five = builder.CreateConstant(Type::U32, 5);

    // X << 0 -> X
    auto* res1 = builder.CreateShl(arg, zero);
    EXPECT_EQ(res1, arg);

    // 0 << X -> 0
    auto* res2 = builder.CreateShl(zero, five);
    ASSERT_EQ(res2->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(res2)->GetValue(), 0);
}

TEST(Optimization, ComplexSubgraphFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 10);
    auto* c2 = builder.CreateConstant(Type::U32, 20);
    auto* c3 = builder.CreateConstant(Type::U32, 5);

    // (10 + 20) -> 30
    auto* t1 = builder.CreateAdd(c1, c2); 
    // (5 << 1) -> 10
    auto* t2 = builder.CreateShl(c3, builder.CreateConstant(Type::U32, 1)); 

    // 30 + 10 -> 40
    auto* res = builder.CreateAdd(t1, t2);

    ASSERT_EQ(res->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(res)->GetValue(), 40);
}

