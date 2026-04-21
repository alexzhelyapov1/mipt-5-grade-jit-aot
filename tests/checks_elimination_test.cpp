#include "ir/opt/checks_elimination.h"
#include "ir/ir_builder.h"
#include "ir/graph.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include <gtest/gtest.h>
#include <algorithm>

using namespace opt;

class ChecksEliminationTest : public ::testing::Test {
protected:
    size_t CountOpcode(const Graph& graph, Opcode opcode) {
        size_t count = 0;
        for (const auto& bb : graph.GetBlocks()) {
            for (auto* inst = bb.GetFirstInstruction(); inst != nullptr; inst = inst->GetNext()) {
                if (inst->GetOpcode() == opcode) {
                    count++;
                }
            }
        }
        return count;
    }
};

TEST_F(ChecksEliminationTest, RedundantNullCheck) {
    Graph graph;
    IRBuilder builder(&graph);

    auto* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* obj = builder.CreateArgument(Type::U64);
    builder.CreateNullCheck(obj);
    builder.CreateNullCheck(obj); // Redundant
    builder.CreateRet(nullptr);

    EXPECT_EQ(CountOpcode(graph, Opcode::NULL_CHECK), 2);

    ChecksElimination ce(&graph);
    ce.Run();

    EXPECT_EQ(CountOpcode(graph, Opcode::NULL_CHECK), 1);
}

TEST_F(ChecksEliminationTest, DominatedNullCheck) {
    Graph graph;
    IRBuilder builder(&graph);

    auto* bb0 = graph.CreateBasicBlock();
    auto* bb1 = graph.CreateBasicBlock();
    auto* bb2 = graph.CreateBasicBlock();

    builder.SetInsertPoint(bb0);
    auto* obj = builder.CreateArgument(Type::U64);
    builder.CreateNullCheck(obj);
    auto* cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, bb1, bb2);

    builder.SetInsertPoint(bb1);
    builder.CreateNullCheck(obj); // Redundant, dominated by bb0
    builder.CreateRet(nullptr);

    builder.SetInsertPoint(bb2);
    builder.CreateRet(nullptr);

    EXPECT_EQ(CountOpcode(graph, Opcode::NULL_CHECK), 2);

    ChecksElimination ce(&graph);
    ce.Run();

    EXPECT_EQ(CountOpcode(graph, Opcode::NULL_CHECK), 1);
}

TEST_F(ChecksEliminationTest, RedundantBoundsCheck) {
    Graph graph;
    IRBuilder builder(&graph);

    auto* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* index = builder.CreateArgument(Type::U32);
    auto* len = builder.CreateConstant(Type::U32, 10);
    builder.CreateBoundsCheck(index, len);
    builder.CreateBoundsCheck(index, len); // Redundant
    builder.CreateRet(nullptr);

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 2);

    ChecksElimination ce(&graph);
    ce.Run();

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 1);
}

TEST_F(ChecksEliminationTest, MustThrowNullCheck) {
    Graph graph;
    IRBuilder builder(&graph);

    auto* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* null_obj = builder.CreateConstant(Type::U64, 0);
    builder.CreateNullCheck(null_obj); // Must throw
    builder.CreateRet(nullptr);        // Should be removed

    EXPECT_EQ(CountOpcode(graph, Opcode::NULL_CHECK), 1);
    EXPECT_EQ(CountOpcode(graph, Opcode::RET), 1);

    ChecksElimination ce(&graph);
    ce.Run();

    EXPECT_EQ(CountOpcode(graph, Opcode::NULL_CHECK), 0);
    EXPECT_EQ(CountOpcode(graph, Opcode::DEOPTIMIZE), 1);
    EXPECT_EQ(CountOpcode(graph, Opcode::RET), 0);
    EXPECT_TRUE(bb->GetSuccessors().empty());
}

TEST_F(ChecksEliminationTest, MustThrowBoundsCheck) {
    Graph graph;
    IRBuilder builder(&graph);

    auto* bb = graph.CreateBasicBlock();
    builder.SetInsertPoint(bb);

    auto* index = builder.CreateConstant(Type::U32, 10);
    auto* len = builder.CreateConstant(Type::U32, 10);
    builder.CreateBoundsCheck(index, len); // Must throw (index >= len)
    builder.CreateRet(nullptr);

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 1);

    ChecksElimination ce(&graph);
    ce.Run();

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 0);
    EXPECT_EQ(CountOpcode(graph, Opcode::DEOPTIMIZE), 1);
}

TEST_F(ChecksEliminationTest, LoopBoundsCheckElimination) {
    Graph graph;
    IRBuilder builder(&graph);

    // BB0 (preheader) -> BB1 (header) -> BB2 (body) -> BB1
    //                                 -> BB3 (exit)
    auto* bb0 = graph.CreateBasicBlock();
    auto* bb1 = graph.CreateBasicBlock();
    auto* bb2 = graph.CreateBasicBlock();
    auto* bb3 = graph.CreateBasicBlock();

    builder.SetInsertPoint(bb0);
    auto* init = builder.CreateConstant(Type::U32, 0);
    auto* len = builder.CreateConstant(Type::U32, 100);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb1);
    auto* phi = builder.CreatePhi(Type::U32);
    auto* cmp = builder.CreateCmp(ConditionCode::LT, phi, len);
    builder.CreateBranch(cmp, bb2, bb3);

    builder.SetInsertPoint(bb2);
    builder.CreateBoundsCheck(phi, len); // Redundant: 0 <= phi < 100
    auto* one = builder.CreateConstant(Type::U32, 1);
    auto* next = builder.CreateAdd(phi, one);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb3);
    builder.CreateRet(nullptr);

    phi->AddIncoming(init, bb0);
    phi->AddIncoming(next, bb2);

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 1);

    ChecksElimination ce(&graph);
    ce.Run();

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 0);
}

TEST_F(ChecksEliminationTest, LoopBoundsCheckPreservation) {
    Graph graph;
    IRBuilder builder(&graph);

    auto* bb0 = graph.CreateBasicBlock();
    auto* bb1 = graph.CreateBasicBlock();
    auto* bb2 = graph.CreateBasicBlock();
    auto* bb3 = graph.CreateBasicBlock();

    builder.SetInsertPoint(bb0);
    auto* init = builder.CreateConstant(Type::U32, 0);
    auto* len = builder.CreateConstant(Type::U32, 100);
    auto* small_len = builder.CreateConstant(Type::U32, 50);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb1);
    auto* phi = builder.CreatePhi(Type::U32);
    auto* cmp = builder.CreateCmp(ConditionCode::LT, phi, len);
    builder.CreateBranch(cmp, bb2, bb3);

    builder.SetInsertPoint(bb2);
    builder.CreateBoundsCheck(phi, small_len); // NOT redundant: phi can be 50..99
    auto* one = builder.CreateConstant(Type::U32, 1);
    auto* next = builder.CreateAdd(phi, one);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb3);
    builder.CreateRet(nullptr);

    phi->AddIncoming(init, bb0);
    phi->AddIncoming(next, bb2);

    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 1);

    ChecksElimination ce(&graph);
    ce.Run();

    // Should still be 1 because loop goes up to 100, but check is for 50
    EXPECT_EQ(CountOpcode(graph, Opcode::BOUNDS_CHECK), 1);
}
