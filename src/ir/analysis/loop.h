#pragma once

#include "ir/basic_block.h"
#include <vector>
#include <unordered_set>
#include <iosfwd>

class Loop {
public:
    explicit Loop(BasicBlock* header);
    
    void AddBlock(BasicBlock* block);
    void AddInnerLoop(Loop* inner_loop);
    void AddBackEdge(BasicBlock* latch);
    
    BasicBlock* GetHeader() const { return header_; }
    const std::vector<BasicBlock*>& GetBlocks() const { return blocks_; }
    const std::vector<Loop*>& GetInnerLoops() const { return inner_loops_; }
    Loop* GetOuterLoop() const { return outer_loop_; }
    const std::vector<BasicBlock*>& GetBackEdges() const { return latches_; }
    bool IsReducible() const { return is_reducible_; }
    bool IsCountable() const { return is_countable_; }
    
    void SetOuterLoop(Loop* outer) { outer_loop_ = outer; }
    void SetReducible(bool reducible) { is_reducible_ = reducible; }
    void SetCountable(bool countable) { is_countable_ = countable; }
    
    bool ContainsBlock(BasicBlock* block) const;
    bool ContainsLoop(Loop* loop) const;
    void Dump(std::ostream& os, int indent = 0) const;

private:
    BasicBlock* header_;
    std::vector<BasicBlock*> blocks_;
    std::vector<BasicBlock*> latches_;
    std::vector<Loop*> inner_loops_;
    Loop* outer_loop_ = nullptr;
    bool is_reducible_ = true;
    bool is_countable_ = false;
};
