#include <gtest/gtest.h>
#include "ir/ir.h"

TEST(IRCreation, GraphAndBasicBlock) {
    Graph graph;

    BasicBlock* bb = graph.CreateBasicBlock();

    ASSERT_NE(bb, nullptr);

    EXPECT_EQ(bb->GetGraph(), &graph);

    EXPECT_EQ(graph.GetBlocks().size(), 1);

    EXPECT_EQ(bb->GetId(), 0);

    BasicBlock* bb2 = graph.CreateBasicBlock();
    ASSERT_NE(bb2, nullptr);
    EXPECT_EQ(bb2->GetId(), 1);
    EXPECT_EQ(graph.GetBlocks().size(), 2);
}

TEST(IRBuilder, CreateSimpleInstructions) {
    Graph graph;
    IRBuilder builder(&graph);

    BasicBlock* basic_block = graph.CreateBasicBlock();
    builder.SetInsertPoint(basic_block);

    ConstantInst* const_40 = builder.CreateConstant(Type::S32, 40);
    ConstantInst* const_2 = builder.CreateConstant(Type::S32, 2);
    BinaryInst* add_inst = builder.CreateAdd(const_40, const_2);

    ASSERT_NE(basic_block->GetFirstInstruction(), nullptr);

    auto* inst1 = basic_block->GetFirstInstruction();
    ASSERT_EQ(inst1->GetOpcode(), Opcode::Constant);
    auto* c1 = static_cast<ConstantInst*>(inst1);
    EXPECT_EQ(c1->GetValue(), 40);

    auto* inst2 = inst1->GetNext();
    ASSERT_NE(inst2, nullptr);
    ASSERT_EQ(inst2->GetOpcode(), Opcode::Constant);
    auto* c2 = static_cast<ConstantInst*>(inst2);
    EXPECT_EQ(c2->GetValue(), 2);

    auto* inst3 = inst2->GetNext();
    ASSERT_NE(inst3, nullptr);
    EXPECT_EQ(inst3->GetOpcode(), Opcode::ADD);

    EXPECT_EQ(basic_block->GetLastInstruction(), inst3);
    EXPECT_EQ(inst3->GetNext(), nullptr);

    const auto& inputs = inst3->GetInputs();
    ASSERT_EQ(inputs.size(), 2);
    EXPECT_EQ(inputs[0], const_40);
    EXPECT_EQ(inputs[1], const_2);
}

TEST(IRBuilder, FunctionWithArguments) {
    Graph graph;
    IRBuilder builder(&graph);

    ArgumentInst* arg_a = builder.CreateArgument(Type::U64);
    ArgumentInst* arg_b = builder.CreateArgument(Type::U64);

    BasicBlock* entry_bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(entry_bb);

    BinaryInst* sum_inst = builder.CreateAdd(arg_a, arg_b);

    builder.CreateRet(sum_inst);

    const auto& args = graph.GetArguments();
    ASSERT_EQ(args.size(), 2);
    EXPECT_EQ(args[0], arg_a);
    EXPECT_EQ(args[1], arg_b);

    const auto& sum_inputs = sum_inst->GetInputs();
    ASSERT_EQ(sum_inputs.size(), 2);
    EXPECT_EQ(sum_inputs[0], arg_a);
    EXPECT_EQ(sum_inputs[1], arg_b);

    User* user_of_a = arg_a->GetFirstUser();
    ASSERT_NE(user_of_a, nullptr);
    EXPECT_EQ(user_of_a->GetUserInstruction(), sum_inst);
    EXPECT_EQ(user_of_a->GetNextUser(), nullptr);

    User* user_of_b = arg_b->GetFirstUser();
    ASSERT_NE(user_of_b, nullptr);
    EXPECT_EQ(user_of_b->GetUserInstruction(), sum_inst);
    EXPECT_EQ(user_of_b->GetNextUser(), nullptr);
}

TEST(IRBuilder, BranchUpdatesCFG) {
    Graph graph;
    IRBuilder builder(&graph);
    auto* entry_bb = graph.CreateBasicBlock();
    auto* true_bb = graph.CreateBasicBlock();
    auto* false_bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(entry_bb);
    auto* cond_true = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond_true, true_bb, false_bb);
    ASSERT_EQ(entry_bb->GetSuccessors().size(), 2);
    EXPECT_EQ(entry_bb->GetSuccessors()[0], true_bb);
    EXPECT_EQ(entry_bb->GetSuccessors()[1], false_bb);
    ASSERT_EQ(true_bb->GetPredecessors().size(), 1);
    EXPECT_EQ(true_bb->GetPredecessors()[0], entry_bb);
    ASSERT_EQ(false_bb->GetPredecessors().size(), 1);
    EXPECT_EQ(false_bb->GetPredecessors()[0], entry_bb);
}

TEST(IRBuilder, CastInstructionProperties) {
    Graph graph;
    IRBuilder builder(&graph);
    auto* basic_block = graph.CreateBasicBlock();
    builder.SetInsertPoint(basic_block);
    auto* arg = builder.CreateArgument(Type::U32);
    auto* casted = builder.CreateCast(Type::U64, arg);
    EXPECT_EQ(casted->GetType(), Type::U64);
    const auto& ins = casted->GetInputs();
    ASSERT_EQ(ins.size(), 1u);
    EXPECT_EQ(ins[0], arg);
}

TEST(IRBuilder, PhiAddIncomingInvalidPredThrows) {
    Graph graph;
    IRBuilder builder(&graph);
    auto* entry_bb = graph.CreateBasicBlock();
    auto* loop_bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(loop_bb);
    auto* phi = builder.CreatePhi(Type::U32);
    auto* val = builder.CreateConstant(Type::U32, 0);
    ASSERT_THROW(phi->AddIncoming(val, entry_bb), std::runtime_error);
}
