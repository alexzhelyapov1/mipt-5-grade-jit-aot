#include <gtest/gtest.h>
#include "ir/ir.h"
#include "ir/opt/peephole_optimizer.h"

void RunOptimization(Graph* graph) {
    PeepholeOptimizer pass(graph);
    pass.Run();
}

TEST(Optimization, AddConstantFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 10);
    auto* c2 = builder.CreateConstant(Type::U32, 32);
    auto* add_inst = builder.CreateAdd(c1, c2);
    auto* ret = builder.CreateRet(add_inst);

    RunOptimization(&graph);

    auto* ret_input = ret->GetInputs()[0];
    ASSERT_EQ(ret_input->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(ret_input)->GetValue(), 42);
}

TEST(Optimization, AddPeepholes) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* arg = builder.CreateArgument(Type::U32);
    auto* zero = builder.CreateConstant(Type::U32, 0);

    // Case 1: X + 0 -> X
    auto* add1 = builder.CreateAdd(arg, zero);
    auto* ret1 = builder.CreateRet(add1);

    // Case 2: 0 + X -> X
    auto* add2 = builder.CreateAdd(zero, arg);
    auto* ret2 = builder.CreateRet(add2);

    RunOptimization(&graph);

    EXPECT_EQ(ret1->GetInputs()[0], arg);
    EXPECT_EQ(ret2->GetInputs()[0], arg);
}

TEST(Optimization, AndConstantFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 0b1100);
    auto* c2 = builder.CreateConstant(Type::U32, 0b1010);
    auto* res = builder.CreateAnd(c1, c2);
    auto* ret = builder.CreateRet(res);

    RunOptimization(&graph);

    auto* val = ret->GetInputs()[0];
    ASSERT_EQ(val->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(val)->GetValue(), 0b1000);
}

TEST(Optimization, AndPeepholes) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* arg = builder.CreateArgument(Type::U32);
    auto* zero = builder.CreateConstant(Type::U32, 0);
    auto* all_ones = builder.CreateConstant(Type::U32, -1);

    // Case 1: X & 0 -> 0
    auto* and1 = builder.CreateAnd(arg, zero);
    auto* ret1 = builder.CreateRet(and1);

    // Case 2: 0 & X -> 0
    auto* and2 = builder.CreateAnd(zero, arg);
    auto* ret2 = builder.CreateRet(and2);

    // Case 3: X & X -> X
    auto* and3 = builder.CreateAnd(arg, arg);
    auto* ret3 = builder.CreateRet(and3);

    // Case 4: X & -1 -> X
    auto* and4 = builder.CreateAnd(arg, all_ones);
    auto* ret4 = builder.CreateRet(and4);

    RunOptimization(&graph);

    // Check 1
    auto* val1 = ret1->GetInputs()[0];
    ASSERT_EQ(val1->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(val1)->GetValue(), 0);

    // Check 2
    auto* val2 = ret2->GetInputs()[0];
    ASSERT_EQ(val2->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(val2)->GetValue(), 0);

    // Check 3
    EXPECT_EQ(ret3->GetInputs()[0], arg);

    // Check 4
    EXPECT_EQ(ret4->GetInputs()[0], arg);
}

TEST(Optimization, ShlConstantFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 3);
    auto* c2 = builder.CreateConstant(Type::U32, 2);
    auto* res = builder.CreateShl(c1, c2);
    auto* ret = builder.CreateRet(res);

    RunOptimization(&graph);

    auto* val = ret->GetInputs()[0];
    ASSERT_EQ(val->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(val)->GetValue(), 12);
}

TEST(Optimization, ShlPeepholes) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* arg = builder.CreateArgument(Type::U32);
    auto* zero = builder.CreateConstant(Type::U32, 0);
    auto* five = builder.CreateConstant(Type::U32, 5);

    // Case 1: X << 0 -> X
    auto* shl1 = builder.CreateShl(arg, zero);
    auto* ret1 = builder.CreateRet(shl1);

    // Case 2: 0 << X -> 0
    auto* shl2 = builder.CreateShl(zero, five);
    auto* ret2 = builder.CreateRet(shl2);

    RunOptimization(&graph);

    // Check 1
    EXPECT_EQ(ret1->GetInputs()[0], arg);

    // Check 2
    auto* val2 = ret2->GetInputs()[0];
    ASSERT_EQ(val2->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(val2)->GetValue(), 0);
}

TEST(Optimization, ComplexSubgraphFolding) {
    Graph graph;
    IRBuilder builder(&graph);
    BasicBlock* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c1 = builder.CreateConstant(Type::U32, 10);
    auto* c2 = builder.CreateConstant(Type::U32, 20);
    auto* c3 = builder.CreateConstant(Type::U32, 5);
    auto* one = builder.CreateConstant(Type::U32, 1);

    // (10 + 20) -> 30
    auto* t1 = builder.CreateAdd(c1, c2);
    // (5 << 1) -> 10
    auto* t2 = builder.CreateShl(c3, one);
    // 30 + 10 -> 40
    auto* res = builder.CreateAdd(t1, t2);

    auto* ret = builder.CreateRet(res);

    RunOptimization(&graph);

    auto* final_val = ret->GetInputs()[0];
    ASSERT_EQ(final_val->GetOpcode(), Opcode::Constant);
    EXPECT_EQ(static_cast<ConstantInst*>(final_val)->GetValue(), 40);
}
