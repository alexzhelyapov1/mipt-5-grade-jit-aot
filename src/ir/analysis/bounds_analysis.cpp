#include "ir/analysis/bounds_analysis.h"
#include "ir/analysis/loop.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include <algorithm>

namespace analysis {

static ConditionCode InvertConditionCode(ConditionCode cc) {
    switch (cc) {
    case ConditionCode::EQ: return ConditionCode::NE;
    case ConditionCode::NE: return ConditionCode::EQ;
    case ConditionCode::LT: return ConditionCode::GE;
    case ConditionCode::GT: return ConditionCode::LE;
    case ConditionCode::LE: return ConditionCode::GT;
    case ConditionCode::GE: return ConditionCode::LT;
    case ConditionCode::UGT: return ConditionCode::ULE;
    case ConditionCode::ULE: return ConditionCode::UGT;
    }
    return cc;
}

static ConditionCode SwapConditionCode(ConditionCode cc) {
    switch (cc) {
        case ConditionCode::LT:
            return ConditionCode::GT;
        case ConditionCode::GT:
            return ConditionCode::LT;
        case ConditionCode::LE:
            return ConditionCode::GE;
        case ConditionCode::GE:
            return ConditionCode::LE;
        case ConditionCode::UGT:
            return ConditionCode::LT;
        case ConditionCode::ULE:
            return ConditionCode::GE;
        default:
            return cc;
    }
}

void BoundsAnalysis::Run(LoopAnalyzer *analyzer) {
    for (Loop *loop : analyzer->GetLoops()) {
        AnalyzeLoop(loop);
    }
}

void BoundsAnalysis::AnalyzeLoop(Loop *loop) {
    if (!loop->IsCountable()) {
        return;
    }

    BasicBlock *header = loop->GetHeader();
    BasicBlock *latch = loop->GetBackEdges()[0];

    Instruction *last_inst = header->GetLastInstruction();
    bool in_header = true;
    if (last_inst == nullptr || last_inst->GetOpcode() != Opcode::JA) {
        last_inst = latch->GetLastInstruction();
        in_header = false;
    }

    if (last_inst == nullptr || last_inst->GetOpcode() != Opcode::JA) {
        loop->SetCountable(false);
        return;
    }

    auto *branch = static_cast<BranchInst *>(last_inst);
    Instruction *cond = branch->GetInputs()[0];

    if (cond->GetOpcode() != Opcode::CMP) {
        loop->SetCountable(false);
        return;
    }

    auto *cmp = static_cast<CompareInst *>(cond);
    Instruction *lhs = cmp->GetInputs()[0];
    Instruction *rhs = cmp->GetInputs()[1];
    ConditionCode cc = cmp->GetCC();

    // We want condition to stay inside the loop.
    if (in_header) {
        bool true_outside = !loop->ContainsBlock(branch->GetTrueBB());
        if (true_outside) {
            cc = InvertConditionCode(cc);
        }
    }

    PhiInst *phi = (lhs->GetOpcode() == Opcode::PHI) ? static_cast<PhiInst *>(lhs) : nullptr;
    Instruction *bound = rhs;
    if (phi == nullptr || phi->GetBasicBlock() != header) {
        phi = (rhs->GetOpcode() == Opcode::PHI) ? static_cast<PhiInst *>(rhs) : nullptr;
        if (phi != nullptr && phi->GetBasicBlock() == header) {
            bound = lhs;
            cc = SwapConditionCode(cc);
        } else {
            phi = nullptr;
        }
    }

    if (phi == nullptr) {
        loop->SetCountable(false);
        return;
    }

    if (phi->GetInputs().size() != 2) {
        loop->SetCountable(false);
        return;
    }

    BasicBlock *outside_bb = nullptr;
    BasicBlock *inside_bb = nullptr;
    Instruction *init_val = nullptr;
    Instruction *update_val = nullptr;

    const auto &preds = header->GetPredecessors();
    for (size_t i = 0; i < preds.size(); ++i) {
        BasicBlock *pred = preds[i];
        if (loop->ContainsBlock(pred)) {
            inside_bb = pred;
            update_val = phi->GetInputs()[i];
        } else {
            outside_bb = pred;
            init_val = phi->GetInputs()[i];
        }
    }

    if (outside_bb == nullptr || inside_bb == nullptr || init_val == nullptr || update_val == nullptr) {
        loop->SetCountable(false);
        return;
    }

    if (update_val->GetOpcode() != Opcode::ADD) {
        loop->SetCountable(false);
        return;
    }

    Instruction *step_lhs = update_val->GetInputs()[0];
    Instruction *step_rhs = update_val->GetInputs()[1];
    Instruction *step_phi = (step_lhs == phi) ? step_lhs : ((step_rhs == phi) ? step_rhs : nullptr);
    Instruction *step_val = (step_lhs == phi) ? step_rhs : ((step_rhs == phi) ? step_lhs : nullptr);

    if (step_phi == nullptr || step_val == nullptr || step_val->GetOpcode() != Opcode::Constant) {
        loop->SetCountable(false);
        return;
    }

    LoopBounds bounds;
    bounds.init = init_val;
    bounds.test = bound;
    bounds.step = step_val;
    bounds.cc = cc;
    bounds.is_countable = true;
    loop_bounds_[header] = bounds;
}

const LoopBounds *BoundsAnalysis::GetLoopBounds(Loop *loop) const {
    auto it = loop_bounds_.find(loop->GetHeader());
    if (it != loop_bounds_.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace analysis
