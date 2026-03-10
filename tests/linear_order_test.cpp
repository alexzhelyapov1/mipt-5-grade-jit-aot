#include "ir/analysis/linear_order.h"
#include "ir/ir.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

using namespace analysis;

static std::vector<uint32_t> GetBlockIds(const std::vector<BasicBlock *> &blocks) {
    std::vector<uint32_t> ids;
    ids.reserve(blocks.size());
    for (const auto *bb : blocks) {
        ids.push_back(bb->GetId());
    }
    return ids;
}

TEST(LinearOrder, StraightLine) {
    Graph graph;
    IRBuilder builder(&graph);

    // BB0 -> BB1 -> BB2
    auto *bb0 = graph.CreateBasicBlock();
    auto *bb1 = graph.CreateBasicBlock();
    auto *bb2 = graph.CreateBasicBlock();

    builder.SetInsertPoint(bb0);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb1);
    builder.CreateJump(bb2);

    builder.SetInsertPoint(bb2);
    builder.CreateRet(nullptr);

    LinearOrder linear_order(&graph);
    const auto &ordered_blocks = linear_order.GetBlocks();

    // RPO should be the same
    std::vector<uint32_t> expected_ids = {bb0->GetId(), bb1->GetId(), bb2->GetId()};
    EXPECT_EQ(GetBlockIds(ordered_blocks), expected_ids);
}

// diamond structure
TEST(LinearOrder, IfElse) {
    Graph graph;
    IRBuilder builder(&graph);

    //    BB0
    //   /
    // BB1   BB2
    //   \   /
    //    BB3
    auto *bb0 = graph.CreateBasicBlock(); // Entry
    auto *bb1 = graph.CreateBasicBlock(); // If
    auto *bb2 = graph.CreateBasicBlock(); // Else
    auto *bb3 = graph.CreateBasicBlock(); // Merge

    builder.SetInsertPoint(bb0);
    auto *cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, bb1, bb2);

    builder.SetInsertPoint(bb1);
    builder.CreateJump(bb3);

    builder.SetInsertPoint(bb2);
    builder.CreateJump(bb3);

    builder.SetInsertPoint(bb3);
    builder.CreateRet(nullptr);

    LinearOrder linear_order(&graph);
    const auto &ordered_blocks = linear_order.GetBlocks();
    auto ordered_ids = GetBlockIds(ordered_blocks);

    // entry block first, merge block last
    ASSERT_EQ(ordered_ids.size(), 4);
    EXPECT_EQ(ordered_ids[0], bb0->GetId());
    EXPECT_EQ(ordered_ids[3], bb3->GetId());

    // Check that BB1 and BB2 are in the middle
    bool bb1_found = std::find(ordered_ids.begin(), ordered_ids.end(), bb1->GetId()) != ordered_ids.end();
    bool bb2_found = std::find(ordered_ids.begin(), ordered_ids.end(), bb2->GetId()) != ordered_ids.end();
    EXPECT_TRUE(bb1_found);
    EXPECT_TRUE(bb2_found);
}

// simple loop
TEST(LinearOrder, SimpleLoop) {
    Graph graph;
    IRBuilder builder(&graph);

    // BB0 -> BB1 -> BB2 -> BB1
    //        ^      /
    //        |-----/
    //        |
    //        v
    //       BB3
    auto *bb0 = graph.CreateBasicBlock(); // Pre-header
    auto *bb1 = graph.CreateBasicBlock(); // Loop Header
    auto *bb2 = graph.CreateBasicBlock(); // Loop Body
    auto *bb3 = graph.CreateBasicBlock(); // Exit

    builder.SetInsertPoint(bb0);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb1);
    auto *cond = builder.CreateConstant(Type::BOOL, 1);
    builder.CreateBranch(cond, bb2, bb3);

    builder.SetInsertPoint(bb2);
    builder.CreateJump(bb1);

    builder.SetInsertPoint(bb3);
    builder.CreateRet(nullptr);

    LinearOrder linear_order(&graph);
    const auto &ordered_blocks = linear_order.GetBlocks();
    auto ordered_ids = GetBlockIds(ordered_blocks);

    // Expected for a reducible loop: Pre-header, then Header. The rest can vary.
    ASSERT_GE(ordered_ids.size(), 2);
    EXPECT_EQ(ordered_ids[0], bb0->GetId());
    EXPECT_EQ(ordered_ids[1], bb1->GetId());
}
