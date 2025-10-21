#pragma once

#include <vector>
#include <cstdint>
#include <iosfwd>

class Graph;
class Instruction;

class BasicBlock {
public:
    BasicBlock(uint32_t id, Graph* parent) : id_(id), parent_graph_(parent) {}

    uint32_t GetId() const { return id_; }
    Graph* GetGraph() const { return parent_graph_; }

    const std::vector<BasicBlock*>& GetPredecessors() const { return predecessors_; }
    const std::vector<BasicBlock*>& GetSuccessors() const { return successors_; }

    // TODO: Need to add field first_phi_inst_ and first_non_phi_inst_, and similar methods to get them
    Instruction* GetFirstInstruction() const { return first_inst_; }
    Instruction* GetLastInstruction() const { return last_inst_; }

    void PushBackInstruction(Instruction* inst);
    void AddPredecessor(BasicBlock* pred);
    void AddSuccessor(BasicBlock* succ);
    void Dump(std::ostream& os) const;

private:
    friend class Graph;
    friend class IRBuilder;

    uint32_t id_;
    Graph* parent_graph_ = nullptr;
    std::vector<BasicBlock*> predecessors_;
    // TODO: successors number is limited to 2, so it's better to use a pair of pointers
    std::vector<BasicBlock*> successors_;
    Instruction* first_inst_ = nullptr;
    Instruction* last_inst_ = nullptr;
};


