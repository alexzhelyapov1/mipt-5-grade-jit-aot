#pragma once

#include "ir/types.h"
#include "ir/graph.h"
#include "ir/analysis/loop_analyzer.h"
#include <unordered_map>

namespace analysis {

struct LoopBounds {
    Instruction *init = nullptr;
    Instruction *test = nullptr;
    Instruction *step = nullptr;
    ConditionCode cc;
    bool is_countable = false;
};

class BoundsAnalysis {
  public:
    explicit BoundsAnalysis(Graph *graph) : graph_(graph) {}

    void Run(LoopAnalyzer *analyzer);

    const LoopBounds *GetLoopBounds(Loop *loop) const;

  private:
    Graph *graph_;
    std::unordered_map<BasicBlock *, LoopBounds> loop_bounds_;

    void AnalyzeLoop(Loop *loop);
};

} // namespace analysis
