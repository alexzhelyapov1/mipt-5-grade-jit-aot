#include "ir/ir_builder.h"
#include "ir/basic_block.h"
#include "ir/graph.h"
#include "ir/instruction.h"
#include <stdexcept>

IRBuilder::IRBuilder(Graph *graph) : graph_(graph) {}

void IRBuilder::SetInsertPoint(BasicBlock *bb) {
    insert_bb_ = bb;
    insert_before_ = nullptr;
}

void IRBuilder::SetInsertPoint(Instruction *inst) {
    insert_bb_ = inst->GetBasicBlock();
    insert_before_ = inst;
}

template <typename InstType, typename... Args> InstType *IRBuilder::CreateInstruction(Args &&...args) {
    if (!insert_bb_) {
        throw std::runtime_error("Insert point not set in IRBuilder");
    }

    auto inst_ptr = std::make_unique<InstType>(graph_->next_inst_id_++, std::forward<Args>(args)...);

    auto *raw_ptr = inst_ptr.get();
    graph_->instructions_.push_back(std::move(inst_ptr));

    if (insert_before_) {
        insert_bb_->InsertBefore(raw_ptr, insert_before_);
    } else {
        insert_bb_->PushBackInstruction(raw_ptr);
    }

    auto &inputs = raw_ptr->GetInputs();
    for (uint32_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i]) {
            graph_->RegisterUse(inputs[i], raw_ptr, i);
        }
    }
    return raw_ptr;
}

ConstantInst *IRBuilder::CreateConstant(Type type, uint64_t value) {
    return CreateInstruction<ConstantInst>(type, value);
}

static ConstantInst *AsConstant(Instruction *inst) { return dynamic_cast<ConstantInst *>(inst); }

BinaryInst *IRBuilder::CreateAdd(Instruction *lhs, Instruction *rhs) {
    return CreateInstruction<BinaryInst>(Opcode::ADD, lhs->GetType(), lhs, rhs);
}

BinaryInst *IRBuilder::CreateMul(Instruction *lhs, Instruction *rhs) {
    return CreateInstruction<BinaryInst>(Opcode::MUL, lhs->GetType(), lhs, rhs);
}

BinaryInst *IRBuilder::CreateAnd(Instruction *lhs, Instruction *rhs) {
    return CreateInstruction<BinaryInst>(Opcode::AND, lhs->GetType(), lhs, rhs);
}

BinaryInst *IRBuilder::CreateShl(Instruction *lhs, Instruction *rhs) {
    return CreateInstruction<BinaryInst>(Opcode::SHL, lhs->GetType(), lhs, rhs);
}

CompareInst *IRBuilder::CreateCmp(ConditionCode cc, Instruction *lhs, Instruction *rhs) {
    return CreateInstruction<CompareInst>(Type::BOOL, cc, lhs, rhs);
}

JumpInst *IRBuilder::CreateJump(BasicBlock *target) {
    auto *jump_inst = CreateInstruction<JumpInst>(target);
    insert_bb_->AddSuccessor(target);
    target->AddPredecessor(insert_bb_);
    return jump_inst;
}

BranchInst *IRBuilder::CreateBranch(Instruction *cond, BasicBlock *true_bb, BasicBlock *false_bb) {
    auto *branch_inst = CreateInstruction<BranchInst>(cond, true_bb, false_bb);
    insert_bb_->AddSuccessor(true_bb);
    insert_bb_->AddSuccessor(false_bb);
    true_bb->AddPredecessor(insert_bb_);
    false_bb->AddPredecessor(insert_bb_);
    return branch_inst;
}

ReturnInst *IRBuilder::CreateRet(Instruction *value) { return CreateInstruction<ReturnInst>(value); }

ArgumentInst *IRBuilder::CreateArgument(Type type) {
    if (!graph_)
        throw std::runtime_error("Graph is not set");
    auto arg_ptr = std::make_unique<ArgumentInst>(graph_->next_inst_id_++, type);
    auto *raw_ptr = arg_ptr.get();
    graph_->instructions_.push_back(std::move(arg_ptr));
    graph_->args_.push_back(raw_ptr);
    return raw_ptr;
}

CastInst *IRBuilder::CreateCast(Type to_type, Instruction *from) { return CreateInstruction<CastInst>(to_type, from); }

PhiInst *IRBuilder::CreatePhi(Type type) {
    if (!insert_bb_) {
        throw std::runtime_error("Insert point not set in IRBuilder for Phi");
    }
    auto phi_ptr = std::make_unique<PhiInst>(graph_->next_inst_id_++, type);
    auto *raw_ptr = phi_ptr.get();
    graph_->instructions_.push_back(std::move(phi_ptr));

    raw_ptr->basic_block_ = insert_bb_;
    auto *first_inst = insert_bb_->first_inst_;
    if (first_inst == nullptr) {
        insert_bb_->first_inst_ = raw_ptr;
        insert_bb_->last_inst_ = raw_ptr;
    } else {
        raw_ptr->next_ = first_inst;
        first_inst->prev_ = raw_ptr;
        insert_bb_->first_inst_ = raw_ptr;
    }

    return raw_ptr;
}

MoveInst *IRBuilder::CreateMove(Type type, Instruction *from) { return CreateInstruction<MoveInst>(type, from); }

LoadInst *IRBuilder::CreateLoad(Type type, Instruction *from) { return CreateInstruction<LoadInst>(type, from); }

StoreInst *IRBuilder::CreateStore(Type type, Instruction *value, Instruction *to) {
    return CreateInstruction<StoreInst>(type, value, to);
}

CallStaticInst *IRBuilder::CreateCallStatic(Graph *callee, const std::vector<Instruction *> &args) {
    return CreateInstruction<CallStaticInst>(callee, args);
}
