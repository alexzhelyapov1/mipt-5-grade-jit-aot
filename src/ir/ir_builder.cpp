#include "ir/ir_builder.h"
#include "ir/graph.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include <stdexcept>

IRBuilder::IRBuilder(Graph* graph) : graph_(graph) {}

void IRBuilder::SetInsertPoint(BasicBlock* bb) {
    insert_point_ = bb;
}

template<typename InstType, typename... Args>
InstType* IRBuilder::CreateInstruction(Args&&... args) {
    if (!insert_point_) {
        throw std::runtime_error("Insert point not set in IRBuilder");
    }

    auto inst_ptr = std::make_unique<InstType>(graph_->next_inst_id_++, std::forward<Args>(args)...);

    auto* raw_ptr = inst_ptr.get();
    graph_->instructions_.push_back(std::move(inst_ptr));
    insert_point_->PushBackInstruction(raw_ptr);

    auto& inputs = raw_ptr->GetInputs();
    for (uint32_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i]) {
            graph_->RegisterUse(inputs[i], raw_ptr, i);
        }
    }
    return raw_ptr;
}

ConstantInst* IRBuilder::CreateConstant(Type type, uint64_t value) {
    return CreateInstruction<ConstantInst>(type, value);
}

// Helper
static ConstantInst* AsConstant(Instruction* inst) {
    return dynamic_cast<ConstantInst*>(inst);
}

Instruction* IRBuilder::CreateAdd(Instruction* lhs, Instruction* rhs) {
    auto* lc = AsConstant(lhs);
    auto* rc = AsConstant(rhs);

    // Constant Folding: Const + Const
    if (lc && rc) {
        return CreateConstant(lhs->GetType(), lc->GetValue() + rc->GetValue());
    }

    // Peephole: X + 0 -> X
    if (rc && rc->GetValue() == 0) {
        return lhs;
    }

    // Peephole: 0 + X -> X
    if (lc && lc->GetValue() == 0) {
        return rhs;
    }

    return CreateInstruction<BinaryInst>(Opcode::ADD, lhs->GetType(), lhs, rhs);
}

Instruction* IRBuilder::CreateMul(Instruction* lhs, Instruction* rhs) {
    return CreateInstruction<BinaryInst>(Opcode::MUL, lhs->GetType(), lhs, rhs);
}

Instruction* IRBuilder::CreateAnd(Instruction* lhs, Instruction* rhs) {
    auto* lc = AsConstant(lhs);
    auto* rc = AsConstant(rhs);

    // Constant Folding
    if (lc && rc) {
        return CreateConstant(lhs->GetType(), lc->GetValue() & rc->GetValue());
    }

    // Peephole: X & 0 -> 0 (но нужно создать новую константу 0 или вернуть существующую)
    if (rc && rc->GetValue() == 0) {
        return rc; 
    }
    if (lc && lc->GetValue() == 0) {
        return lc;
    }

    // Peephole: X & X -> X
    if (lhs == rhs) {
        return lhs;
    }

    // Peephole: X & -1 (all ones) -> X
    if (rc && rc->GetValue() == static_cast<uint64_t>(-1)) {
        return lhs;
    }

    return CreateInstruction<BinaryInst>(Opcode::AND, lhs->GetType(), lhs, rhs);
}

Instruction* IRBuilder::CreateShl(Instruction* lhs, Instruction* rhs) {
    auto* lc = AsConstant(lhs);
    auto* rc = AsConstant(rhs);

    // Constant Folding
    if (lc && rc) {
        return CreateConstant(lhs->GetType(), lc->GetValue() << rc->GetValue());
    }

    // Peephole: X << 0 -> X
    if (rc && rc->GetValue() == 0) {
        return lhs;
    }

    // Peephole: 0 << X -> 0
    if (lc && lc->GetValue() == 0) {
        return lc;
    }

    return CreateInstruction<BinaryInst>(Opcode::SHL, lhs->GetType(), lhs, rhs);
}

CompareInst* IRBuilder::CreateCmp(ConditionCode cc, Instruction* lhs, Instruction* rhs) {
    return CreateInstruction<CompareInst>(Type::BOOL, cc, lhs, rhs);
}

void IRBuilder::CreateJump(BasicBlock* target) {
    auto* jump_inst = CreateInstruction<JumpInst>(target);
    insert_point_->AddSuccessor(target);
    target->AddPredecessor(insert_point_);
}

void IRBuilder::CreateBranch(Instruction* cond, BasicBlock* true_bb, BasicBlock* false_bb) {
    auto* branch_inst = CreateInstruction<BranchInst>(cond, true_bb, false_bb);
    insert_point_->AddSuccessor(true_bb);
    insert_point_->AddSuccessor(false_bb);
    true_bb->AddPredecessor(insert_point_);
    false_bb->AddPredecessor(insert_point_);
}

void IRBuilder::CreateRet(Instruction* value) {
    CreateInstruction<ReturnInst>(value);
}

ArgumentInst* IRBuilder::CreateArgument(Type type) {
    if (!graph_) throw std::runtime_error("Graph is not set");
    auto arg_ptr = std::make_unique<ArgumentInst>(graph_->next_inst_id_++, type);
    auto* raw_ptr = arg_ptr.get();
    graph_->instructions_.push_back(std::move(arg_ptr));
    graph_->args_.push_back(raw_ptr);
    return raw_ptr;
}

CastInst* IRBuilder::CreateCast(Type to_type, Instruction* from) {
    return CreateInstruction<CastInst>(to_type, from);
}

PhiInst* IRBuilder::CreatePhi(Type type) {
    if (!insert_point_) {
        throw std::runtime_error("Insert point not set in IRBuilder for Phi");
    }
    auto phi_ptr = std::make_unique<PhiInst>(graph_->next_inst_id_++, type);
    auto* raw_ptr = phi_ptr.get();
    graph_->instructions_.push_back(std::move(phi_ptr));

    raw_ptr->basic_block_ = insert_point_;
    auto* first_inst = insert_point_->first_inst_;
    if (first_inst == nullptr) {
        insert_point_->first_inst_ = raw_ptr;
        insert_point_->last_inst_ = raw_ptr;
    } else {
        raw_ptr->next_ = first_inst;
        first_inst->prev_ = raw_ptr;
        insert_point_->first_inst_ = raw_ptr;
    }

    return raw_ptr;
}
