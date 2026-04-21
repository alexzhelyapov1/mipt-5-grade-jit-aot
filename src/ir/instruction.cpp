#include "ir/instruction.h"
#include "ir/basic_block.h"
#include "ir/graph.h"
#include <algorithm>
#include <ostream>
#include <stdexcept>

static const char *OpcodeToString(Opcode op) {
    switch (op) {
    case Opcode::Constant:
        return "Constant";
    case Opcode::Argument:
        return "Param";
    case Opcode::ADD:
        return "Add";
    case Opcode::MUL:
        return "Mul";
    case Opcode::AND:
        return "And";
    case Opcode::SHL:
        return "Shl";
    case Opcode::CMP:
        return "Cmp";
    case Opcode::JUMP:
        return "Jump";
    case Opcode::JA:
        return "Branch";
    case Opcode::RET:
        return "Ret";
    case Opcode::PHI:
        return "Phi";
    case Opcode::U32_TO_U64:
        return "U32ToU64";
    case Opcode::CAST:
        return "Cast";
    case Opcode::MOVE:
        return "Move";
    case Opcode::LOAD:
        return "Load";
    case Opcode::STORE:
        return "Store";
    case Opcode::CALL_STATIC:
        return "CallStatic";
    case Opcode::NULL_CHECK:
        return "NullCheck";
    case Opcode::BOUNDS_CHECK:
        return "BoundsCheck";
    case Opcode::DEOPTIMIZE:
        return "Deoptimize";
    }
    return "Unknown";
}

static const char *TypeToString(Type t) {
    switch (t) {
    case Type::VOID:
        return "void";
    case Type::BOOL:
        return "bool";
    case Type::U32:
        return "u32";
    case Type::S32:
        return "s32";
    case Type::U64:
        return "u64";
    }
    return "<unknown-ty>";
}

static const char *ConditionCodeToString(ConditionCode cc) {
    switch (cc) {
    case ConditionCode::EQ:
        return "eq";
    case ConditionCode::NE:
        return "ne";
    case ConditionCode::LT:
        return "lt";
    case ConditionCode::GT:
        return "gt";
    case ConditionCode::LE:
        return "le";
    case ConditionCode::GE:
        return "ge";
    case ConditionCode::UGT:
        return "ugt";
    case ConditionCode::ULE:
        return "ule";
    }
    return "<unknown-cc>";
}

static void PrintInputs(std::ostream &os, const std::vector<Instruction *> &inputs) {
    os << "(";
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (i)
            os << ", ";
        if (inputs[i]) {
            os << "i" << inputs[i]->GetId();
        } else {
            os << "-";
        }
    }
    os << ")";
}

static void PrintUsers(std::ostream &os, const Instruction *inst) {
    os << " -> (";
    bool first = true;
    for (User *u = inst->GetFirstUser(); u != nullptr; u = u->GetNextUser()) {
        if (!first)
            os << ", ";
        first = false;
        os << "i" << u->GetUserInstruction()->GetId();
    }
    os << ")";
}

void Instruction::Print(std::ostream &os) const {
    const bool is_phi = (GetOpcode() == Opcode::PHI);
    os << "i" << GetId() << (is_phi ? "p" : "") << "." << TypeToString(GetType()) << " " << OpcodeToString(GetOpcode())
       << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void ConstantInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Constant " << GetValue();
    PrintUsers(os, this);
}

void BinaryInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " " << OpcodeToString(GetOpcode()) << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void CompareInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Cmp(" << ConditionCodeToString(cc_) << ") ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void BranchInst::Print(std::ostream &os) const {
    os << "branch i" << GetInputs()[0]->GetId() << " to BB" << true_bb_->GetId() << ", BB" << false_bb_->GetId();
}

void JumpInst::Print(std::ostream &os) const { os << "jump BB" << target_bb_->GetId(); }

void ReturnInst::Print(std::ostream &os) const {
    if (!GetInputs().empty() && GetInputs()[0]) {
        os << "ret i" << GetInputs()[0]->GetId();
    } else {
        os << "ret";
    }
}

void ArgumentInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Argument";
    PrintUsers(os, this);
}

void CastInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Cast ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void PhiInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "p." << TypeToString(GetType()) << " Phi ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void MoveInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " " << OpcodeToString(GetOpcode()) << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void LoadInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " " << OpcodeToString(GetOpcode()) << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void StoreInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " " << OpcodeToString(GetOpcode()) << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void CallStaticInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " CallStatic ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void NullCheckInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " NullCheck ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void BoundsCheckInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " BoundsCheck ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void DeoptimizeInst::Print(std::ostream &os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Deoptimize ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

Instruction *ConstantInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new ConstantInst(0, GetType(), GetValue());
}

Instruction *BinaryInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new BinaryInst(0, GetOpcode(), GetType(), MapInput(GetInputs()[0], mapping),
                          MapInput(GetInputs()[1], mapping));
}

Instruction *CompareInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new CompareInst(0, GetType(), cc_, MapInput(GetInputs()[0], mapping), MapInput(GetInputs()[1], mapping));
}

Instruction *BranchInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new BranchInst(0, MapInput(GetInputs()[0], mapping), true_bb_, false_bb_);
}

Instruction *JumpInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new JumpInst(0, target_bb_);
}

Instruction *ReturnInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    if (GetInputs().empty()) {
        return new ReturnInst(0);
    }
    return new ReturnInst(0, MapInput(GetInputs()[0], mapping));
}

Instruction *ArgumentInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new ArgumentInst(0, GetType());
}

Instruction *CastInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new CastInst(0, GetType(), MapInput(GetInputs()[0], mapping));
}

Instruction *PhiInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    auto *new_phi = new PhiInst(0, GetType());
    new_phi->ResizeInputs(GetInputs().size());
    for (size_t i = 0; i < GetInputs().size(); ++i) {
        new_phi->SetInput(i, MapInput(GetInputs()[i], mapping));
    }
    return new_phi;
}

Instruction *MoveInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new MoveInst(0, GetType(), MapInput(GetInputs()[0], mapping));
}

Instruction *LoadInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new LoadInst(0, GetType(), MapInput(GetInputs()[0], mapping));
}

Instruction *StoreInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new StoreInst(0, GetType(), MapInput(GetInputs()[0], mapping), MapInput(GetInputs()[1], mapping));
}

Instruction *CallStaticInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    std::vector<Instruction *> new_args;
    for (auto *arg : GetInputs()) {
        new_args.push_back(MapInput(arg, mapping));
    }
    return new CallStaticInst(0, callee_, new_args);
}

Instruction *NullCheckInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new NullCheckInst(0, MapInput(GetInputs()[0], mapping));
}

Instruction *BoundsCheckInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new BoundsCheckInst(0, MapInput(GetInputs()[0], mapping), MapInput(GetInputs()[1], mapping));
}

Instruction *DeoptimizeInst::Clone(Graph *target_graph, const InstMapping &mapping) const {
    return new DeoptimizeInst(0);
}

void PhiInst::AddIncoming(Instruction *value, BasicBlock *pred) {
    auto *parent_bb = GetBasicBlock();
    if (!parent_bb) {
        throw std::runtime_error("PhiInst must be inside a BasicBlock to add incoming values");
    }

    auto &preds = parent_bb->GetPredecessors();
    auto it = std::find(preds.begin(), preds.end(), pred);
    if (it == preds.end()) {
        throw std::runtime_error("Basic block is not a predecessor");
    }
    size_t index = std::distance(preds.begin(), it);

    if (GetInputs().size() <= index) {
        ResizeInputs(index + 1);
    }
    SetInput(index, value);

    parent_bb->GetGraph()->RegisterUse(value, this, static_cast<uint32_t>(index));
}

void Instruction::ReplaceAllUsesWith(Instruction *other_inst) {
    if (this == other_inst)
        return;

    User *current_user = head_user_;
    while (current_user) {
        Instruction *user_inst = current_user->GetUserInstruction();
        uint32_t idx = current_user->GetInputIndex();

        user_inst->SetInput(idx, other_inst);

        if (auto *bb = user_inst->GetBasicBlock()) {
            bb->GetGraph()->RegisterUse(other_inst, user_inst, idx);
        }

        current_user = current_user->GetNextUser();
    }

    head_user_ = nullptr;
}
