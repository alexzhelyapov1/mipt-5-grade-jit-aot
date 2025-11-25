#pragma once

#include "ir/analysis/loop.h"
#include "ir/analysis/graph_analyzer.h"
#include "ir/graph.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>

class LoopAnalyzer {
public:
    explicit LoopAnalyzer(Graph* graph);
    ~LoopAnalyzer();

    void Analyze();

    const std::vector<Loop*>& GetLoops() const { return loops_; }
    Loop* GetLoopForBlock(BasicBlock* block) const;
    const std::vector<Loop*>& GetLoopsForBlock(BasicBlock* block) const;
    Loop* GetRootLoop() const { return root_loop_; }
    bool IsLoopHeader(BasicBlock* block) const;

    void Dump(std::ostream& os) const;

private:
    void CollectBackEdges();
    void PopulateLoops();
    void BuildLoopTree();
    void ClassifyLoops();

    bool IsBackEdge(BasicBlock* from, BasicBlock* to) const;
    void FindLoopBlocks(Loop* loop, BasicBlock* latch);
    void DFSForLoopBlocks(BasicBlock* block, BasicBlock* header,
                         std::unordered_set<BasicBlock*>& visited, Loop* loop);
    void MarkDFS(BasicBlock* block, std::unordered_set<BasicBlock*>& visited,
                 std::unordered_set<BasicBlock*>& in_stack);
    void CheckLoopCountable(Loop* loop);
    bool IsInnerLoop(Loop* inner, Loop* outer) const;
    bool IsDescendant(BasicBlock* parent, BasicBlock* child) const;

    Graph* graph_;
    GraphAnalyzer graph_analyzer_;
    std::vector<Loop*> loops_;
    Loop* root_loop_ = nullptr;

    std::unordered_map<BasicBlock*, Loop*> header_to_loop_;
    std::unordered_map<BasicBlock*, Loop*> block_to_innermost_loop_;
    std::unordered_map<BasicBlock*, std::vector<Loop*>> block_to_all_loops_;
    std::vector<std::pair<BasicBlock*, BasicBlock*>> back_edges_;

    std::unordered_map<BasicBlock*, int> dfs_numbers_;
    std::unordered_map<BasicBlock*, int> dfs_exit_numbers_;
    std::unordered_set<BasicBlock*> in_stack_;
    int dfs_counter_ = 0;
};
