#pragma once

#include <list>
#include <memory>
#include <iosfwd>
#include <vector>

class BasicBlock;
class Instruction;
class User;
class ArgumentInst;

class Graph {
public:
    Graph() = default;
    BasicBlock* CreateBasicBlock();
    const std::list<BasicBlock>& GetBlocks() const { return blocks_; }
    User* RegisterUse(Instruction* def, Instruction* user_inst, uint32_t input_idx);
    void Dump(std::ostream& os) const;
    const auto& GetArguments() const { return args_; }

private:
    friend class IRBuilder;

    std::list<BasicBlock> blocks_;
    std::list<std::unique_ptr<Instruction>> instructions_;
    std::list<std::unique_ptr<User>> users_;

    BasicBlock* start_block_ = nullptr;
    uint32_t next_block_id_ = 0;
    uint32_t next_inst_id_ = 0;
    std::vector<ArgumentInst*> args_;
};


