#pragma once

#include "ir/types.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

class Graph;
class BasicBlock;
class Instruction;
class User;

using InstMapping = std::map<Instruction *, Instruction *>;

inline Instruction *MapInput(Instruction *input, const InstMapping &mapping) {
    if (input == nullptr)
        return nullptr;
    auto it = mapping.find(input);
    if (it != mapping.end()) {
        return it->second;
    }
    return input;
}

namespace opt {
class RegisterAllocator;
}

class Location {
  public:
    enum Kind {
        UNASSIGNED,
        REGISTER,
        STACK,
    };

    Location() : kind_(UNASSIGNED), value_(0) {}

    static Location MakeRegister(int32_t reg_num) { return Location(REGISTER, reg_num); }
    static Location MakeStack(int32_t offset) { return Location(STACK, offset); }

    Kind GetKind() const { return kind_; }
    int32_t GetValue() const { return value_; }

  private:
    Location(Kind kind, int32_t value) : kind_(kind), value_(value){};

    Kind kind_;
    int32_t value_;
};

class User {
  public:
    User(Instruction *user_inst, uint32_t input_idx) : user_inst_(user_inst), input_idx_(input_idx) {}

    Instruction *GetUserInstruction() const { return user_inst_; }
    uint32_t GetInputIndex() const { return input_idx_; }

    User *GetNextUser() const { return next_user_; }
    void SetNextUser(User *user) { next_user_ = user; }

  private:
    Instruction *const user_inst_;
    const uint32_t input_idx_;
    User *next_user_ = nullptr;
};

class Instruction {
  public:
    virtual ~Instruction() = default;

    Opcode GetOpcode() const { return opcode_; }
    Type GetType() const { return type_; }
    uint32_t GetId() const { return id_; }
    void SetId(uint32_t id) { id_ = id; }
    BasicBlock *GetBasicBlock() const { return basic_block_; }
    void SetBasicBlock(BasicBlock *bb) { basic_block_ = bb; }
    const std::vector<Instruction *> &GetInputs() const { return inputs_; }
    User *GetFirstUser() const { return head_user_; }

    Instruction *GetNext() const { return next_; }
    Instruction *GetPrev() const { return prev_; }

    Location GetLocation() const { return location_; }
    void SetLocation(Location loc) { location_ = loc; }

    virtual void Print(std::ostream &os) const;
    void ReplaceAllUsesWith(Instruction *other_inst);
    virtual Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const = 0;

    void ClearInputs() { inputs_.clear(); }
    void AddInput(Instruction *input) { inputs_.push_back(input); }
    void ResizeInputs(size_t new_size) { inputs_.resize(new_size); }
    void SetInput(size_t idx, Instruction *inst) { inputs_[idx] = inst; }

  protected:
    Instruction(Opcode opcode, Type type, uint32_t id) : opcode_(opcode), type_(type), id_(id) {}

    Opcode opcode_;
    Type type_;
    uint32_t id_;
    Location location_;

    BasicBlock *basic_block_ = nullptr;
    Instruction *prev_ = nullptr;
    Instruction *next_ = nullptr;
    std::vector<Instruction *> inputs_;
    User *head_user_ = nullptr;

  private:
    Instruction(const Instruction &) = delete;
    Instruction &operator=(const Instruction &) = delete;

    friend class BasicBlock;
    friend class IRBuilder;
    friend class PhiInst;
    friend class Graph;
    friend class opt::RegisterAllocator;
};

class ConstantInst : public Instruction {
  public:
    ConstantInst(uint32_t id, Type type, uint64_t val) : Instruction(Opcode::Constant, type, id), value_(val) {}

    uint64_t GetValue() const { return value_; }

    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;

  private:
    uint64_t value_;
};

class BinaryInst : public Instruction {
  public:
    BinaryInst(uint32_t id, Opcode opcode, Type type, Instruction *lhs, Instruction *rhs)
        : Instruction(opcode, type, id) {
        AddInput(lhs);
        AddInput(rhs);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class CompareInst : public Instruction {
  public:
    CompareInst(uint32_t id, Type type, ConditionCode cc, Instruction *lhs, Instruction *rhs)
        : Instruction(Opcode::CMP, type, id), cc_(cc) {
        AddInput(lhs);
        AddInput(rhs);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;

    ConditionCode GetCC() const { return cc_; }

  private:
    ConditionCode cc_;
};

class TerminatorInst : public Instruction {
  protected:
    TerminatorInst(uint32_t id, Opcode opcode) : Instruction(opcode, Type::VOID, id) {}
};

class BranchInst : public TerminatorInst {
  public:
    BranchInst(uint32_t id, Instruction *cond, BasicBlock *true_bb, BasicBlock *false_bb)
        : TerminatorInst(id, Opcode::JA), true_bb_(true_bb), false_bb_(false_bb) {
        AddInput(cond);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;

    BasicBlock *GetTrueBB() const { return true_bb_; }
    BasicBlock *GetFalseBB() const { return false_bb_; }
    void SetTrueBB(BasicBlock *bb) { true_bb_ = bb; }
    void SetFalseBB(BasicBlock *bb) { false_bb_ = bb; }

  private:
    BasicBlock *true_bb_;
    BasicBlock *false_bb_;
};

class JumpInst : public TerminatorInst {
  public:
    JumpInst(uint32_t id, BasicBlock *target_bb) : TerminatorInst(id, Opcode::JUMP), target_bb_(target_bb) {}
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;

    BasicBlock *GetTarget() const { return target_bb_; }
    void SetTarget(BasicBlock *bb) { target_bb_ = bb; }

  private:
    BasicBlock *target_bb_;
};

class ReturnInst : public TerminatorInst {
  public:
    ReturnInst(uint32_t id, Instruction *value) : TerminatorInst(id, Opcode::RET) { AddInput(value); }
    ReturnInst(uint32_t id) : TerminatorInst(id, Opcode::RET) {}
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class ArgumentInst : public Instruction {
  public:
    ArgumentInst(uint32_t id, Type type) : Instruction(Opcode::Argument, type, id) {}
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class CastInst : public Instruction {
  public:
    CastInst(uint32_t id, Type to_type, Instruction *from_inst) : Instruction(Opcode::CAST, to_type, id) {
        AddInput(from_inst);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class PhiInst : public Instruction {
  public:
    PhiInst(uint32_t id, Type type) : Instruction(Opcode::PHI, type, id) {}
    void AddIncoming(Instruction *value, BasicBlock *pred);
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class MoveInst : public Instruction {
  public:
    MoveInst(uint32_t id, Type type, Instruction *from) : Instruction(Opcode::MOVE, type, id) { AddInput(from); }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class LoadInst : public Instruction {
  public:
    LoadInst(uint32_t id, Type type, Instruction *from) : Instruction(Opcode::LOAD, type, id) { AddInput(from); }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class StoreInst : public Instruction {
  public:
    StoreInst(uint32_t id, Type type, Instruction *value, Instruction *to) : Instruction(Opcode::STORE, type, id) {
        AddInput(value);
        AddInput(to);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class CallStaticInst : public Instruction {
  public:
    CallStaticInst(uint32_t id, Graph *callee, const std::vector<Instruction *> &args)
        : Instruction(Opcode::CALL_STATIC, Type::VOID, id), callee_(callee) {
        for (auto *arg : args) {
            AddInput(arg);
        }
    }

    Graph *GetCallee() const { return callee_; }
    void SetReturnType(Type type) { type_ = type; }

    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;

  private:
    Graph *callee_;
};

class NullCheckInst : public Instruction {
  public:
    NullCheckInst(uint32_t id, Instruction *obj): Instruction(Opcode::NULL_CHECK, Type::VOID, id) {
      AddInput(obj);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class BoundsCheckInst : public Instruction {
  public:
    BoundsCheckInst(uint32_t id, Instruction *index, Instruction *len)
        : Instruction(Opcode::BOUNDS_CHECK, Type::VOID, id) {
        AddInput(index);
        AddInput(len);
    }
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};

class DeoptimizeInst : public TerminatorInst {
  public:
    DeoptimizeInst(uint32_t id) : TerminatorInst(id, Opcode::DEOPTIMIZE) {}
    void Print(std::ostream &os) const override;
    Instruction *Clone(Graph *target_graph, const InstMapping &mapping) const override;
};
