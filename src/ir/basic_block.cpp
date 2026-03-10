#include "ir/basic_block.h"
#include "ir/instruction.h"
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

void BasicBlock::AddPredecessor(BasicBlock *pred) { predecessors_.push_back(pred); }

void BasicBlock::AddSuccessor(BasicBlock *succ) { successors_.push_back(succ); }

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
