#include "ir/analysis/loop_analyzer.h"
#include "ir/basic_block.h"
#include "ir/graph.h"
#include <algorithm>
#include <iostream>
#include <stack>

LoopAnalyzer::LoopAnalyzer(Graph* graph) 
    : graph_(graph), graph_analyzer_(graph) {}

LoopAnalyzer::~LoopAnalyzer() {
    for (Loop* loop : loops_) {
        delete loop;
    }
    delete root_loop_;
}

void LoopAnalyzer::Analyze() {
    graph_analyzer_.BuildDominatorTree();
    CollectBackEdges();
    PopulateLoops();
    BuildLoopTree();
    ClassifyLoops();
}

void LoopAnalyzer::CollectBackEdges() {
    dfs_numbers_.clear();
    in_stack_.clear();
    back_edges_.clear();
    dfs_counter_ = 0;
    
    std::unordered_set<BasicBlock*> visited;
    if (!graph_analyzer_.GetReversePostOrder().empty()) {
        MarkDFS(graph_analyzer_.GetReversePostOrder()[0], visited, in_stack_);
    }
}

void LoopAnalyzer::MarkDFS(BasicBlock* block, std::unordered_set<BasicBlock*>& visited, 
                           std::unordered_set<BasicBlock*>& in_stack) {
    visited.insert(block);
    in_stack.insert(block);
    dfs_numbers_[block] = dfs_counter_++;
    
    for (BasicBlock* succ : block->GetSuccessors()) {
        if (in_stack.count(succ)) {
            if (dfs_numbers_[block] > dfs_numbers_[succ]) {
                back_edges_.emplace_back(block, succ);
            }
        } else if (!visited.count(succ)) {
            MarkDFS(succ, visited, in_stack);
        }
    }
    
    in_stack.erase(block);
}

void LoopAnalyzer::PopulateLoops() {
    for (auto& back_edge : back_edges_) {
        BasicBlock* latch = back_edge.first;
        BasicBlock* header = back_edge.second;
        
        Loop* loop = nullptr;
        if (header_to_loop_.count(header)) {
            loop = header_to_loop_[header];
        } else {
            loop = new Loop(header);
            loops_.push_back(loop);
            header_to_loop_[header] = loop;
            
            bool is_reducible = graph_analyzer_.Dominates(header, latch);
            loop->SetReducible(is_reducible);
        }
        
        loop->AddBackEdge(latch);
        
        if (loop->IsReducible()) {
            FindLoopBlocks(loop, latch);
        } else {
            for (auto& edge : back_edges_) {
                if (edge.second == header) {
                    loop->AddBlock(edge.first);
                }
            }
        }
    }
}

void LoopAnalyzer::FindLoopBlocks(Loop* loop, BasicBlock* latch) {
    std::stack<BasicBlock*> stack;
    std::unordered_set<BasicBlock*> visited;
    
    stack.push(latch);
    visited.insert(latch);
    
    BasicBlock* header = loop->GetHeader();
    
    while (!stack.empty()) {
        BasicBlock* current = stack.top();
        stack.pop();
        
        if (current == header) {
            continue;
        }
        
        if (!loop->ContainsBlock(current)) {
            loop->AddBlock(current);
        }
        
        for (BasicBlock* pred : current->GetPredecessors()) {
            if (!visited.count(pred) && graph_analyzer_.Dominates(header, pred)) {
                visited.insert(pred);
                stack.push(pred);
            }
        }
    }
}


void LoopAnalyzer::BuildLoopTree() {
    root_loop_ = new Loop(nullptr);
    
    std::unordered_set<BasicBlock*> all_blocks;
    for (const auto& bb : graph_->GetBlocks()) {
        all_blocks.insert(const_cast<BasicBlock*>(&bb));
    }
    
    for (BasicBlock* block : all_blocks) {
        Loop* innermost_loop = nullptr;
        
        for (Loop* loop : loops_) {
            if (loop->ContainsBlock(block)) {
                if (innermost_loop == nullptr) {
                    innermost_loop = loop;
                } else {
                    if (IsInnerLoop(loop, innermost_loop)) {
                        innermost_loop = loop;
                    }
                }
            }
        }
        
        if (innermost_loop != nullptr) {
            block_to_innermost_loop_[block] = innermost_loop;
            
            std::vector<Loop*> loops_for_block;
            for (Loop* loop : loops_) {
                if (loop->ContainsBlock(block)) {
                    loops_for_block.push_back(loop);
                }
            }
            std::sort(loops_for_block.begin(), loops_for_block.end(), 
                     [this](Loop* a, Loop* b) {
                         return !IsInnerLoop(a, b);
                     });
            block_to_all_loops_[block] = loops_for_block;
        } else {
            root_loop_->AddBlock(block);
        }
    }
    
    for (Loop* loop : loops_) {
        Loop* outer = nullptr;
        
        for (Loop* candidate : loops_) {
            if (candidate != loop && candidate->ContainsBlock(loop->GetHeader())) {
                if (outer == nullptr || IsInnerLoop(candidate, outer)) {
                    outer = candidate;
                }
            }
        }
        
        if (outer != nullptr) {
            if (!outer->ContainsLoop(loop)) {
                outer->AddInnerLoop(loop);
            }
        } else {
            if (!root_loop_->ContainsLoop(loop)) {
                root_loop_->AddInnerLoop(loop);
            }
        }
    }
}

bool LoopAnalyzer::IsInnerLoop(Loop* inner, Loop* outer) const {
    if (outer->ContainsBlock(inner->GetHeader())) {
        return true;
    }
    
    Loop* parent = inner->GetOuterLoop();
    while (parent != nullptr) {
        if (parent == outer) {
            return true;
        }
        parent = parent->GetOuterLoop();
    }
    
    return false;
}

void LoopAnalyzer::ClassifyLoops() {
    for (Loop* loop : loops_) {
        CheckLoopCountable(loop);
    }
}

void LoopAnalyzer::CheckLoopCountable(Loop* loop) {
    
    if (!loop->IsReducible()) {
        loop->SetCountable(false);
        return;
    }
    
    if (loop->GetBackEdges().size() != 1) {
        loop->SetCountable(false);
        return;
    }
    
    
    loop->SetCountable(true);
}

Loop* LoopAnalyzer::GetLoopForBlock(BasicBlock* block) const {
    auto it = block_to_innermost_loop_.find(block);
    return it != block_to_innermost_loop_.end() ? it->second : nullptr;
}

const std::vector<Loop*>& LoopAnalyzer::GetLoopsForBlock(BasicBlock* block) const {
    static std::vector<Loop*> empty;
    auto it = block_to_all_loops_.find(block);
    if (it != block_to_all_loops_.end()) {
        return it->second;
    }
    return empty;
}

bool LoopAnalyzer::IsLoopHeader(BasicBlock* block) const {
    return header_to_loop_.count(block) > 0;
}

void LoopAnalyzer::Dump(std::ostream& os) const {
    os << "Loop Analysis Results:\n";
    os << "=====================\n";
    
    os << "Back edges:\n";
    for (const auto& edge : back_edges_) {
        os << "  BB" << edge.first->GetId() << " -> BB" << edge.second->GetId() << "\n";
    }
    
    os << "\nLoops:\n";
    if (loops_.empty()) {
        os << "  (none)\n";
    } else {
        for (const Loop* loop : loops_) {
            loop->Dump(os, 2);
            os << "\n";
        }
    }
    
    if (root_loop_ && !root_loop_->GetBlocks().empty()) {
        os << "Root loop (blocks outside any loop):\n";
        root_loop_->Dump(os, 2);
    }
}
