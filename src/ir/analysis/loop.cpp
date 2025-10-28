#include "ir/analysis/loop.h"
#include <algorithm>
#include <iostream>

Loop::Loop(BasicBlock* header) : header_(header) {
    blocks_.push_back(header);
}

void Loop::AddBlock(BasicBlock* block) {
    if (std::find(blocks_.begin(), blocks_.end(), block) == blocks_.end()) {
        blocks_.push_back(block);
    }
}

void Loop::AddInnerLoop(Loop* inner_loop) {
    if (std::find(inner_loops_.begin(), inner_loops_.end(), inner_loop) == inner_loops_.end()) {
        inner_loops_.push_back(inner_loop);
        inner_loop->SetOuterLoop(this);
        
        for (BasicBlock* block : inner_loop->GetBlocks()) {
            AddBlock(block);
        }
    }
}

void Loop::AddBackEdge(BasicBlock* latch) {
    if (std::find(latches_.begin(), latches_.end(), latch) == latches_.end()) {
        latches_.push_back(latch);
    }
}

bool Loop::ContainsBlock(BasicBlock* block) const {
    return std::find(blocks_.begin(), blocks_.end(), block) != blocks_.end();
}

bool Loop::ContainsLoop(Loop* loop) const {
    if (std::find(inner_loops_.begin(), inner_loops_.end(), loop) != inner_loops_.end()) {
        return true;
    }
    
    for (Loop* inner : inner_loops_) {
        if (inner->ContainsLoop(loop)) {
            return true;
        }
    }
    
    return false;
}

void Loop::Dump(std::ostream& os, int indent) const {
    std::string indent_str(indent, ' ');
    os << indent_str << "Loop (header: BB" << header_->GetId() 
       << ", reducible: " << (is_reducible_ ? "true" : "false")
       << ", countable: " << (is_countable_ ? "true" : "false") << ")\n";
    os << indent_str << "  Blocks: ";
    for (size_t i = 0; i < blocks_.size(); ++i) {
        if (i > 0) {
            os << ", ";
        }
        os << "BB" << blocks_[i]->GetId();
    }
    os << "\n";
    os << indent_str << "  Latches: ";
    for (size_t i = 0; i < latches_.size(); ++i) {
        if (i > 0) {
            os << ", ";
        }
        os << "BB" << latches_[i]->GetId();
    }
    os << "\n";
    for (const Loop* inner : inner_loops_) {
        inner->Dump(os, indent + 2);
    }
}
