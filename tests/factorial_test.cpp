#include <gtest/gtest.h>
#include "ir/ir.h"
#include "helpers/factorial_graph.h"

TEST(Factorial, GraphStructure) {
    Graph graph;
    BuildFactorialGraph(&graph);

    ASSERT_EQ(graph.GetBlocks().size(), 4);

    auto it = graph.GetBlocks().begin();
    const BasicBlock* entry_bb = &(*it++);
    const BasicBlock* loop_bb = &(*it++);
    const BasicBlock* body_bb = &(*it++);
    const BasicBlock* exit_bb = &(*it++);

    ASSERT_EQ(entry_bb->GetSuccessors().size(), 1);
    EXPECT_EQ(entry_bb->GetSuccessors()[0], loop_bb);

    ASSERT_EQ(loop_bb->GetSuccessors().size(), 2);
    EXPECT_EQ(loop_bb->GetSuccessors()[0], body_bb);
    EXPECT_EQ(loop_bb->GetSuccessors()[1], exit_bb);
    EXPECT_EQ(loop_bb->GetPredecessors().size(), 2);

    ASSERT_EQ(body_bb->GetSuccessors().size(), 1);
    EXPECT_EQ(body_bb->GetSuccessors()[0], loop_bb);

    ASSERT_EQ(exit_bb->GetSuccessors().size(), 0);
    EXPECT_EQ(exit_bb->GetPredecessors().size(), 1);

    ASSERT_EQ(graph.GetArguments().size(), 1);
    EXPECT_EQ(graph.GetArguments()[0]->GetType(), Type::U32);
}