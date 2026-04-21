#include "ir/basic_block.h"
#include "ir/instruction.h"
#include "ir/graph.h"
#include <ostream>

void BasicBlock::PushBackInstruction(Instruction *inst) {
    if (first_inst_ == nullptr) {
        first_inst_ = inst;
        last_inst_ = inst;
    } else {
        last_inst_->next_ = inst;
        inst->prev_ = last_inst_;
        last_inst_ = inst;
    }

    inst->basic_block_ = this;
}

void BasicBlock::InsertBefore(Instruction *new_inst, Instruction *before_inst) {
    new_inst->basic_block_ = this;
    if (before_inst == first_inst_) {
        new_inst->next_ = first_inst_;
        first_inst_->prev_ = new_inst;
        first_inst_ = new_inst;
    } else {
        new_inst->prev_ = before_inst->prev_;
        new_inst->next_ = before_inst;
        before_inst->prev_->next_ = new_inst;
        before_inst->prev_ = new_inst;
    }
}

void BasicBlock::RemoveInstruction(Instruction *inst) {
    if (inst->prev_) {
        inst->prev_->next_ = inst->next_;
    } else {
        first_inst_ = inst->next_;
    }

    if (inst->next_) {
        inst->next_->prev_ = inst->prev_;
    } else {
        last_inst_ = inst->prev_;
    }
    inst->prev_ = inst->next_ = nullptr;
    inst->basic_block_ = nullptr;
}

BasicBlock *BasicBlock::SplitAt(Instruction *inst) {
    BasicBlock *new_bb = parent_graph_->CreateBasicBlock();

    new_bb->successors_ = std::move(successors_);
    for (auto *succ : new_bb->successors_) {
        succ->ReplacePredecessor(this, new_bb);
    }
    successors_.clear();

    new_bb->first_inst_ = inst;
    if (inst == nullptr) {
        new_bb->last_inst_ = nullptr;
    } else {
        new_bb->last_inst_ = last_inst_;
    }

    if (inst && inst->prev_) {
        last_inst_ = inst->prev_;
        last_inst_->next_ = nullptr;
        inst->prev_ = nullptr;
    } else if (inst == first_inst_) {
        first_inst_ = nullptr;
        last_inst_ = nullptr;
    }

    for (Instruction *curr = new_bb->first_inst_; curr != nullptr; curr = curr->next_) {
        curr->basic_block_ = new_bb;
    }

    return new_bb;
}

void BasicBlock::AddPredecessor(BasicBlock *pred) { predecessors_.push_back(pred); }

void BasicBlock::AddSuccessor(BasicBlock *succ) { successors_.push_back(succ); }

void BasicBlock::ReplacePredecessor(BasicBlock *old_pred, BasicBlock *new_pred) {
    for (auto &pred : predecessors_) {
        if (pred == old_pred) {
            pred = new_pred;
        }
    }
}

void BasicBlock::RemovePredecessor(BasicBlock *pred_to_remove) {
    auto it = std::find(predecessors_.begin(), predecessors_.end(), pred_to_remove);
    if (it != predecessors_.end()) {
        predecessors_.erase(it);
    }
}

void BasicBlock::ClearSuccessors() {
    for (auto *succ : successors_) {
        succ->RemovePredecessor(this);
    }
    successors_.clear();
}

static void PrintBlockList(std::ostream &os, const char *label, const std::vector<BasicBlock *> &list) {
    os << label << ":";

    if (list.empty()) {
        os << " -";
    } else {
        for (size_t i = 0; i < list.size(); ++i) {
            os << (i == 0 ? " " : ", ") << "BB" << list[i]->GetId();
        }
    }

    os << '\n';
}

void BasicBlock::Dump(std::ostream &os) const {
    os << "BB" << id_ << ":\n";

    PrintBlockList(os, "  Preds", predecessors_);

    for (auto *inst = first_inst_; inst != nullptr; inst = inst->GetNext()) {
        os << "  ";
        inst->Print(os);
        os << '\n';
    }

    PrintBlockList(os, "  Succs", successors_);
}
