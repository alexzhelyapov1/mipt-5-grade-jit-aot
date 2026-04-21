#pragma once

#include "ir/graph.h"
#include "ir/analysis/graph_analyzer.h"
#include "ir/analysis/bounds_analysis.h"

namespace opt {

class ChecksElimination {
  public:
    explicit ChecksElimination(Graph *graph) : graph_(graph) {}

    void Run();

  private:
    Graph *graph_;

    void EliminateDominatedChecks();
    void EliminateRedundantBoundsChecks();
    void EliminateMustThrowChecks();
    bool Dominates(Instruction *dom, Instruction *inst, const GraphAnalyzer &analyzer);
};

} // namespace opt
