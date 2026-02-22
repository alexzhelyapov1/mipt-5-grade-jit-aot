#include "ir/analysis/liveness_analyzer.h"
#include "ir/ir.h"
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

using namespace analysis;

static bool HasRange(const LiveInterval *interval, uint32_t start, uint32_t end) {
  if (!interval)
    return false;
  for (const auto &range : interval->GetRanges()) {
    if (range.start == start && range.end == end) {
      return true;
    }
  }
  return false;
}

// if-else structure
TEST(LivenessAnalysis, IfElseBranch) {
  Graph graph;
  IRBuilder builder(&graph);

  // BB0 -> BB1
  // BB1 -> BB2, BB3 (if-else)
  // BB2 -> BB4
  // BB3 -> BB4
  // BB4 -> BB5 (phi merge)
  auto *bb0 = graph.CreateBasicBlock(); // Entry
  auto *bb1 = graph.CreateBasicBlock(); // Condition
  auto *bb2 = graph.CreateBasicBlock(); // If-true
  auto *bb3 = graph.CreateBasicBlock(); // Else-false
  auto *bb4 = graph.CreateBasicBlock(); // Merge
  auto *bb5 = graph.CreateBasicBlock(); // Exit

  // BB0: Entry
  builder.SetInsertPoint(bb0);
  auto *initial_val = builder.CreateConstant(Type::U32, 1);
  builder.CreateJump(bb1);

  // BB1: Condition
  builder.SetInsertPoint(bb1);
  auto *cond_val = builder.CreateCmp(ConditionCode::EQ, initial_val, builder.CreateConstant(Type::U32, 1));
  builder.CreateBranch(cond_val, bb2, bb3);

  // BB2: If-true branch
  builder.SetInsertPoint(bb2);
  auto *if_val = builder.CreateConstant(Type::U32, 10);
  builder.CreateJump(bb4);

  // BB3: Else-false branch
  builder.SetInsertPoint(bb3);
  auto *else_val = builder.CreateConstant(Type::U32, 20);
  builder.CreateJump(bb4);

  // BB4: Merge block
  builder.SetInsertPoint(bb4);
  auto *phi_val = builder.CreatePhi(Type::U32);
  phi_val->AddIncoming(if_val, bb2);
  phi_val->AddIncoming(else_val, bb3);
  builder.CreateRet(phi_val);

  // BB5: not used, just to have a clean exit
  builder.SetInsertPoint(bb5);
  builder.CreateRet(nullptr);

  // Analyze the graph
  LivenessAnalyzer analyzer(&graph);
  analyzer.Analyze();

  std::stringstream ss;
  analyzer.Dump(ss);
  // FAIL() << ss.str();

  // Get instruction positions
  auto pos = [&](const Instruction *inst) { return analyzer.GetInstructionPosition(inst); };

  // Get block boundaries
  auto block_end = [&](BasicBlock *bb) {
    return analyzer.GetLinearOrder().size() > bb->GetId()
               ? analyzer.GetInstructionPosition(bb->GetLastInstruction()) + 2
               : 0;
  };

  // --- Verification ---

  auto *i0_interval = analyzer.GetLiveInterval(initial_val);
  ASSERT_NE(i0_interval, nullptr);
  ASSERT_EQ(i0_interval->GetRanges().size(), 1);
  EXPECT_EQ(i0_interval->GetRanges()[0].start, pos(initial_val));
  EXPECT_EQ(i0_interval->GetRanges()[0].end, pos(cond_val));

  auto *i2_interval = analyzer.GetLiveInterval(cond_val);
  ASSERT_NE(i2_interval, nullptr);
  ASSERT_EQ(i2_interval->GetRanges().size(), 1);
  EXPECT_EQ(i2_interval->GetRanges()[0].start, pos(cond_val));
  EXPECT_EQ(i2_interval->GetRanges()[0].end, pos(bb1->GetLastInstruction()));

  auto *i4_interval = analyzer.GetLiveInterval(if_val);
  ASSERT_NE(i4_interval, nullptr);
  ASSERT_EQ(i4_interval->GetRanges().size(), 1);
  EXPECT_EQ(i4_interval->GetRanges()[0].start, pos(if_val));
  EXPECT_EQ(i4_interval->GetRanges()[0].end, block_end(bb2));

  auto *i5_interval = analyzer.GetLiveInterval(else_val);
  ASSERT_NE(i5_interval, nullptr);
  ASSERT_EQ(i5_interval->GetRanges().size(), 1);
  EXPECT_EQ(i5_interval->GetRanges()[0].start, pos(else_val));
  EXPECT_EQ(i5_interval->GetRanges()[0].end, block_end(bb3));

  auto *i6_interval = analyzer.GetLiveInterval(phi_val);
  ASSERT_NE(i6_interval, nullptr);
  ASSERT_EQ(i6_interval->GetRanges().size(), 1);
  EXPECT_EQ(i6_interval->GetRanges()[0].start, pos(phi_val));
  EXPECT_EQ(i6_interval->GetRanges()[0].end, pos(bb4->GetLastInstruction()));
}

// reducible loop
TEST(LivenessAnalysis, SimpleLoop) {
  Graph graph;
  IRBuilder builder(&graph);

  // BB0 -> BB1 (pre-header)
  // BB1 -> BB2 (loop header)
  // BB2 -> BB3 (loop body) -> BB2 (back-edge)
  // BB2 -> BB4 (exit)
  auto *bb0 = graph.CreateBasicBlock(); // Entry
  auto *bb1 = graph.CreateBasicBlock(); // Pre-header
  auto *bb2 = graph.CreateBasicBlock(); // Loop Header
  auto *bb3 = graph.CreateBasicBlock(); // Loop Body
  auto *bb4 = graph.CreateBasicBlock(); // Exit

  // BB0: Entry
  builder.SetInsertPoint(bb0);
  auto *initial_val = builder.CreateConstant(Type::U32, 0);
  builder.CreateJump(bb1);

  // BB1: Pre-header
  builder.SetInsertPoint(bb1);
  builder.CreateJump(bb2);

  // BB2: Loop Header
  builder.SetInsertPoint(bb2);
  auto *phi = builder.CreatePhi(Type::U32);
  auto *cond = builder.CreateCmp(ConditionCode::LT, phi, builder.CreateConstant(Type::U32, 10));
  builder.CreateBranch(cond, bb3, bb4);

  // BB3: Loop Body
  builder.SetInsertPoint(bb3);
  auto *incremented_val = builder.CreateAdd(phi, builder.CreateConstant(Type::U32, 1));
  builder.CreateJump(bb2);

  // Complete the PHI node inputs
  phi->AddIncoming(initial_val, bb1);
  phi->AddIncoming(incremented_val, bb3);

  // BB4: Exit
  builder.SetInsertPoint(bb4);
  builder.CreateRet(phi);

  // Analyze
  LivenessAnalyzer analyzer(&graph);
  analyzer.Analyze();

  auto pos = [&](const Instruction *inst) { return analyzer.GetInstructionPosition(inst); };

  auto block_start = [&](BasicBlock *bb) {
    return analyzer.GetLinearOrder().size() > bb->GetId() ? analyzer.GetInstructionPosition(bb->GetFirstInstruction())
                                                          : 0;
  };
  auto block_end = [&](BasicBlock *bb) {
    return analyzer.GetLinearOrder().size() > bb->GetId()
               ? analyzer.GetInstructionPosition(bb->GetLastInstruction()) + 2
               : 0;
  };

  // --- Verification ---

  auto *phi_interval = analyzer.GetLiveInterval(phi);
  ASSERT_NE(phi_interval, nullptr);
  ASSERT_EQ(phi_interval->GetRanges().size(), 1);
  EXPECT_EQ(phi_interval->GetRanges()[0].start, pos(phi));
  EXPECT_EQ(phi_interval->GetRanges()[0].end, block_end(bb3));

  auto *inc_interval = analyzer.GetLiveInterval(incremented_val);
  ASSERT_NE(inc_interval, nullptr);
  ASSERT_EQ(inc_interval->GetRanges().size(), 1);
  EXPECT_EQ(inc_interval->GetRanges()[0].start, pos(incremented_val));
  EXPECT_EQ(inc_interval->GetRanges()[0].end, block_end(bb3));
}
