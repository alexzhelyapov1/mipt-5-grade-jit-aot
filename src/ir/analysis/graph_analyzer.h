#pragma once

#include "ir/graph.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <stack>

class GraphAnalyzer {
public:
    explicit GraphAnalyzer(Graph* graph) : graph_(graph) {}

    void ComputeRPO();
    void BuildDominatorTree();
    
    const std::vector<BasicBlock*>& GetReversePostOrder() const { return reverse_postorder_; }
    BasicBlock* GetImmediateDominator(BasicBlock* block) const;
    const std::unordered_map<BasicBlock*, size_t>& GetRPONumbers() const { return rpo_numbers_; }

private:
    void DFS(BasicBlock* block, std::unordered_set<BasicBlock*>& visited);

    Graph* graph_;
    BasicBlock* start_block_ = nullptr;
    std::vector<BasicBlock*> reverse_postorder_;
    std::unordered_map<BasicBlock*, BasicBlock*> immediate_dominators_;
    std::unordered_map<BasicBlock*, size_t> rpo_numbers_;
};
