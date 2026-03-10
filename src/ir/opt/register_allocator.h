#pragma once

#include "ir/graph.h"
#include "ir/analysis/liveness_analyzer.h"
#include "ir/ir_builder.h"
#include <vector>
#include <set>

namespace opt {

class RegisterAllocator {
public:
    RegisterAllocator(Graph* graph, uint32_t num_regs);

    void Run();

private:
    void ExpireOldIntervals(analysis::LiveInterval* current);
    void SpillAtInterval(analysis::LiveInterval* current);
    void AllocateRegister(analysis::LiveInterval* current);
    
    void RewriteAndInsertSpillFill();
    void ResolvePhis(IRBuilder& builder);
    void SpillFillInstructions(IRBuilder& builder);
    void SpillFillForInstruction(Instruction* inst, IRBuilder& builder);

    Graph* graph_;
    uint32_t num_regs_;
    uint32_t num_reserved_regs_ = 2;
    uint32_t reserved_regs_start_ = 0;
    std::vector<analysis::LiveInterval*> intervals_;
    std::vector<analysis::LiveInterval*> active_;
    std::set<uint32_t> free_regs_;
    uint32_t stack_offset_ = 0;
};

} // namespace opt
