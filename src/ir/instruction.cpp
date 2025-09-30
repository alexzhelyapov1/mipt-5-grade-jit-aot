#include "ir/instruction.h"
#include "ir/basic_block.h"
#include <stdexcept>
#include <algorithm>
#include "ir/graph.h"
#include <ostream>

static const char* OpcodeToString(Opcode op) {
    switch (op) {
        case Opcode::Constant: return "Constant";
        case Opcode::Argument: return "Param";
        case Opcode::ADD: return "Add";
        case Opcode::MUL: return "Mul";
        case Opcode::CMP: return "Cmp";
        case Opcode::JUMP: return "Jump";
        case Opcode::JA: return "Branch";
        case Opcode::RET: return "Ret";
        case Opcode::PHI: return "Phi";
        case Opcode::U32_TO_U64: return "U32ToU64";
        case Opcode::CAST: return "Cast";
    }
    return "<unknown-op>";
}

static const char* TypeToString(Type t) {
    switch (t) {
        case Type::VOID: return "void";
        case Type::BOOL: return "bool";
        case Type::U32:  return "u32";
        case Type::S32:  return "s32";
        case Type::U64:  return "u64";
    }
    return "<unknown-ty>";
}

static const char* ConditionCodeToString(ConditionCode cc) {
    switch (cc) {
        case ConditionCode::EQ:  return "eq";
        case ConditionCode::NE:  return "ne";
        case ConditionCode::LT:  return "lt";
        case ConditionCode::GT:  return "gt";
        case ConditionCode::LE:  return "le";
        case ConditionCode::GE:  return "ge";
        case ConditionCode::UGT: return "ugt";
        case ConditionCode::ULE: return "ule";
    }
    return "<unknown-cc>";
}

static void PrintInputs(std::ostream& os, const std::vector<Instruction*>& inputs) {
    os << "(";
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (i) os << ", ";
        if (inputs[i]) {
            os << "i" << inputs[i]->GetId();
        } else {
            os << "-";
        }
    }
    os << ")";
}

static void PrintUsers(std::ostream& os, const Instruction* inst) {
    os << " -> (";
    bool first = true;
    for (User* u = inst->GetFirstUser(); u != nullptr; u = u->GetNextUser()) {
        if (!first) os << ", ";
        first = false;
        os << "i" << u->GetUserInstruction()->GetId();
    }
    os << ")";
}

void Instruction::Print(std::ostream& os) const {
    const bool is_phi = (GetOpcode() == Opcode::PHI);
    os << "i" << GetId() << (is_phi ? "p" : "") << "." << TypeToString(GetType())
       << " " << OpcodeToString(GetOpcode()) << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void ConstantInst::Print(std::ostream& os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Constant " << GetValue();
    PrintUsers(os, this);
}

void BinaryInst::Print(std::ostream& os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " " << OpcodeToString(GetOpcode()) << " ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void CompareInst::Print(std::ostream& os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Cmp(" << ConditionCodeToString(cc_) << ") ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void BranchInst::Print(std::ostream& os) const {
    os << "branch i" << GetInputs()[0]->GetId()
       << " to BB" << true_bb_->GetId() << ", BB" << false_bb_->GetId();
}

void JumpInst::Print(std::ostream& os) const {
    os << "jump BB" << target_bb_->GetId();
}

void ReturnInst::Print(std::ostream& os) const {
    if (!GetInputs().empty() && GetInputs()[0]) {
        os << "ret i" << GetInputs()[0]->GetId();
    } else {
        os << "ret";
    }
}

void ArgumentInst::Print(std::ostream& os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Argument";
    PrintUsers(os, this);
}

void CastInst::Print(std::ostream& os) const {
    os << "i" << GetId() << "." << TypeToString(GetType()) << " Cast ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void PhiInst::Print(std::ostream& os) const {
    os << "i" << GetId() << "p." << TypeToString(GetType()) << " Phi ";
    PrintInputs(os, GetInputs());
    PrintUsers(os, this);
}

void PhiInst::AddIncoming(Instruction* value, BasicBlock* pred) {
    auto* parent_bb = GetBasicBlock();
    if (!parent_bb) {
        throw std::runtime_error("PhiInst must be inside a BasicBlock to add incoming values");
    }

    auto& preds = parent_bb->GetPredecessors();
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


