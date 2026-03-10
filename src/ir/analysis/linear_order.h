#pragma once

#include "ir/analysis/graph_analyzer.h"
#include <vector>

class Graph;
class BasicBlock;

namespace analysis {

class LinearOrder {
  public:
    explicit LinearOrder(Graph *graph);

    const std::vector<BasicBlock *> &GetBlocks() const { return linear_order_; }

  private:
    void Build();

    Graph *graph_;
    GraphAnalyzer graph_analyzer_;
    std::vector<BasicBlock *> linear_order_;
};

} // namespace analysis
