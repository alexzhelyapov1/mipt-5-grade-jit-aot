#pragma once

#include "ir/types.h"
#include <cstdint>
#include <vector>
#include <iostream>

class Graph;
class BasicBlock;
class Instruction;
class User;

class User {
public:
    User(Instruction* user_inst, uint32_t input_idx)
        : user_inst_(user_inst), input_idx_(input_idx) {}

    Instruction* GetUserInstruction() const { return user_inst_; }
    uint32_t GetInputIndex() const { return input_idx_; }

    User* GetNextUser() const { return next_user_; }
    void SetNextUser(User* user) { next_user_ = user; }

private:
    Instruction* const user_inst_;
    const uint32_t input_idx_;
    User* next_user_ = nullptr;
};

class Instruction {
public:
    virtual ~Instruction() = default;
    
    Opcode GetOpcode() const { return opcode_; }
    Type GetType() const { return type_; }
    uint32_t GetId() const { return id_; }
    BasicBlock* GetBasicBlock() const { return basic_block_; }
    const std::vector<Instruction*>& GetInputs() const { return inputs_; }
    User* GetFirstUser() const { return head_user_; }

    Instruction* GetNext() const { return next_; }
    Instruction* GetPrev() const { return prev_; }

    virtual void Print(std::ostream& os) const;

protected:
    Instruction(Opcode opcode, Type type, uint32_t id)
        : opcode_(opcode), type_(type), id_(id) {}
    
    void AddInput(Instruction* input) { inputs_.push_back(input); }

    void ResizeInputs(size_t new_size) { inputs_.resize(new_size); }
    void SetInput(size_t idx, Instruction* inst) { inputs_[idx] = inst; }

private:
    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    friend class BasicBlock;
    friend class IRBuilder;
    friend class PhiInst;
    friend class Graph;

    Opcode opcode_;
    Type type_;
    uint32_t id_;

    BasicBlock* basic_block_ = nullptr;
    Instruction* prev_ = nullptr;
    Instruction* next_ = nullptr;
    std::vector<Instruction*> inputs_;
    User* head_user_ = nullptr;
};

class ConstantInst : public Instruction {
public:
    ConstantInst(uint32_t id, Type type, uint64_t val)
        : Instruction(Opcode::Constant, type, id), value_(val) {}
    
    uint64_t GetValue() const { return value_; }

    void Print(std::ostream& os) const override;

private:
    uint64_t value_;
};

class BinaryInst : public Instruction {
public:
    BinaryInst(uint32_t id, Opcode opcode, Type type, Instruction* lhs, Instruction* rhs)
        : Instruction(opcode, type, id) {
        AddInput(lhs);
        AddInput(rhs);
    }
    void Print(std::ostream& os) const override;
};

class CompareInst : public Instruction {
public:
    CompareInst(uint32_t id, Type type, ConditionCode cc, Instruction* lhs, Instruction* rhs)
        : Instruction(Opcode::CMP, type, id), cc_(cc) {
        AddInput(lhs);
        AddInput(rhs);
    }
    void Print(std::ostream& os) const override;
private:
    ConditionCode cc_;
};

class TerminatorInst : public Instruction {
protected:
    TerminatorInst(uint32_t id, Opcode opcode) : Instruction(opcode, Type::VOID, id) {}
};

class BranchInst : public TerminatorInst {
public:
    BranchInst(uint32_t id, Instruction* cond, BasicBlock* true_bb, BasicBlock* false_bb)
        : TerminatorInst(id, Opcode::JA), true_bb_(true_bb), false_bb_(false_bb) {
        AddInput(cond);
    }
    void Print(std::ostream& os) const override;
private:
    BasicBlock* true_bb_;
    BasicBlock* false_bb_;
};

class JumpInst : public TerminatorInst {
public:
    JumpInst(uint32_t id, BasicBlock* target_bb)
        : TerminatorInst(id, Opcode::JUMP), target_bb_(target_bb) {}
    void Print(std::ostream& os) const override;
private:
    BasicBlock* target_bb_;
};

class ReturnInst : public TerminatorInst {
public:
    ReturnInst(uint32_t id, Instruction* value) : TerminatorInst(id, Opcode::RET) { AddInput(value); }
    ReturnInst(uint32_t id) : TerminatorInst(id, Opcode::RET) {}
    void Print(std::ostream& os) const override;
};

class ArgumentInst : public Instruction {
public:
    ArgumentInst(uint32_t id, Type type) : Instruction(Opcode::Argument, type, id) {}
    void Print(std::ostream& os) const override;
};

class CastInst : public Instruction {
public:
    CastInst(uint32_t id, Type to_type, Instruction* from_inst)
        : Instruction(Opcode::CAST, to_type, id) { AddInput(from_inst); }
    void Print(std::ostream& os) const override;
};

class PhiInst : public Instruction {
public:
    PhiInst(uint32_t id, Type type) : Instruction(Opcode::PHI, type, id) {}
    void AddIncoming(Instruction* value, BasicBlock* pred);
    void Print(std::ostream& os) const override;
};


