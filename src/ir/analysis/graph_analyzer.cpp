#include "ir/analysis/graph_analyzer.h"
#include "ir/basic_block.h"
#include <algorithm>
#include <iostream>
#include <queue>

void GraphAnalyzer::ComputeRPO() {
    std::unordered_set<BasicBlock*> visited;
    reverse_postorder_.clear();
    rpo_numbers_.clear();

    if (graph_->GetBlocks().empty()) {
        return;
    }

    const auto& blocks = graph_->GetBlocks();
    BasicBlock* start_block = const_cast<BasicBlock*>(&(*blocks.begin()));
    
    DFS(start_block, visited);

    std::reverse(reverse_postorder_.begin(), reverse_postorder_.end());

    for (size_t i = 0; i < reverse_postorder_.size(); ++i) {
        rpo_numbers_[reverse_postorder_[i]] = i;
    }

    start_block_ = reverse_postorder_.empty() ? nullptr : reverse_postorder_[0];
}

void GraphAnalyzer::DFS(BasicBlock* block, std::unordered_set<BasicBlock*>& visited) {
    if (visited.count(block)) {
        return;
    }
    
    visited.insert(block);
    
    for (BasicBlock* succ : block->GetSuccessors()) {
        DFS(succ, visited);
    }
    
    reverse_postorder_.push_back(block);
}

void GraphAnalyzer::BuildDominatorTree() {
    ComputeRPO();
    
    if (reverse_postorder_.empty()) {
        return;
    }

    BasicBlock* start_block = reverse_postorder_[0];
    immediate_dominators_.clear();

    for (BasicBlock* bb : reverse_postorder_) {
        immediate_dominators_[bb] = nullptr;
    }
    immediate_dominators_[start_block] = start_block;

    bool changed = true;
    while (changed) {
        changed = false;
        
        for (size_t i = 1; i < reverse_postorder_.size(); ++i) {
            BasicBlock* block = reverse_postorder_[i];
            BasicBlock* new_idom = nullptr;

            for (BasicBlock* pred : block->GetPredecessors()) {
                if (immediate_dominators_[pred] != nullptr) {
                    new_idom = pred;
                    break;
                }
            }

            if (new_idom == nullptr) {
                continue;
            }

            for (BasicBlock* pred : block->GetPredecessors()) {
                if (pred != new_idom && immediate_dominators_[pred] != nullptr) {
                    BasicBlock* b = pred;
                    BasicBlock* a = new_idom;
                    
                    while (a != b) {
                        while (rpo_numbers_[a] < rpo_numbers_[b]) {
                            b = immediate_dominators_[b];
                        }
                        while (rpo_numbers_[b] < rpo_numbers_[a]) {
                            a = immediate_dominators_[a];
                        }
                    }
                    new_idom = a;
                }
            }

            if (immediate_dominators_[block] != new_idom) {
                immediate_dominators_[block] = new_idom;
                changed = true;
            }
        }
    }
}

BasicBlock* GraphAnalyzer::GetImmediateDominator(BasicBlock* block) const {
    if (block == start_block_) {
        return nullptr;
    }

    auto it = immediate_dominators_.find(block);
    return it != immediate_dominators_.end() ? it->second : nullptr;
}
