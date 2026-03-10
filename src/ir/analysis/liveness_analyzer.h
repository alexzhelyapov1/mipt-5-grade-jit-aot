#pragma once

#include "ir/analysis/linear_order.h"
#include "ir/analysis/live_interval.h"
#include "ir/analysis/loop_analyzer.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Graph;
class Instruction;
class BasicBlock;

namespace analysis {

class LivenessAnalyzer {
public:
  explicit LivenessAnalyzer(Graph *graph);
  ~LivenessAnalyzer();

  void Analyze();

  LiveInterval *GetLiveInterval(Instruction *inst) const;
  const std::vector<BasicBlock *> &GetLinearOrder() const;
  uint32_t GetInstructionPosition(Instruction *inst) const;

  void Dump(std::ostream &os) const;

private:
  void NumberInstructions();
  LiveInterval *GetOrCreateInterval(Instruction *inst);
  void ProcessBlock(BasicBlock *block,
                    std::unordered_map<BasicBlock *, std::unordered_set<Instruction *>> &live_in_sets);

  Graph *graph_;
  LinearOrder linear_order_;
  LoopAnalyzer loop_analyzer_;

  std::unordered_map<Instruction *, LiveInterval *> intervals_;
  std::unordered_map<BasicBlock *, LiveRange> block_positions_;
  std::unordered_map<Instruction *, uint32_t> inst_positions_;
};

} // namespace analysis
