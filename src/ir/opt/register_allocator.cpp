#include "ir/opt/register_allocator.h"
#include "ir/analysis/liveness_analyzer.h"
#include "ir/analysis/linear_order.h"
#include "ir/ir_builder.h"
#include "ir/graph.h"
#include "ir/instruction.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <set>
#include <list>

namespace opt {

RegisterAllocator::RegisterAllocator(Graph* graph, uint32_t num_total_regs)
    : graph_(graph) {
    if (num_total_regs <= num_reserved_regs_) {
        // Not enough registers to do anything useful
        num_regs_ = 0;
        reserved_regs_start_ = 0;
    } else {
        num_regs_ = num_total_regs - num_reserved_regs_;
        reserved_regs_start_ = num_regs_;
    }

    for (uint32_t i = 0; i < num_regs_; ++i) {
        free_regs_.insert(i);
    }
}

void RegisterAllocator::Run() {
    if (num_regs_ == 0) return; // Nothing to allocate

    analysis::LivenessAnalyzer liveness_analyzer(graph_);
    liveness_analyzer.Analyze();

    for (auto& block : liveness_analyzer.GetLinearOrder()) {
        for (auto* inst = block->GetFirstInstruction(); inst; inst = inst->GetNext()) {
            auto interval = liveness_analyzer.GetLiveInterval(inst);
            if (interval && !interval->GetRanges().empty()) {
                intervals_.push_back(interval);
            }
        }
    }

    std::sort(intervals_.begin(), intervals_.end(), [](auto a, auto b) {
        return a->GetRanges().front().start < b->GetRanges().front().start;
    });

    for (auto& interval : intervals_) {
        ExpireOldIntervals(interval);
        if (active_.size() == num_regs_) {
            SpillAtInterval(interval);
        } else {
            AllocateRegister(interval);
        }
    }

    RewriteAndInsertSpillFill();
}

void RegisterAllocator::ExpireOldIntervals(analysis::LiveInterval* current) {
    std::vector<analysis::LiveInterval*> still_active;
    for (auto* active_interval : active_) {
        if (active_interval->GetRanges().back().end >= current->GetRanges().front().start) {
            still_active.push_back(active_interval);
        } else {
            auto loc = active_interval->GetInstruction()->GetLocation();
            if (loc.GetKind() == Location::REGISTER) {
                free_regs_.insert(loc.GetValue());
            }
        }
    }
    active_ = still_active;
}

static void SortedInsert(std::vector<analysis::LiveInterval*>& vec, analysis::LiveInterval* inst) {
    vec.insert(std::lower_bound(vec.begin(), vec.end(), inst, [](auto a, auto b) {
                   return a->GetRanges().back().end < b->GetRanges().back().end;
               }),
               inst);
}

void RegisterAllocator::SpillAtInterval(analysis::LiveInterval* current) {
    auto spill = active_.back();
    if (spill->GetRanges().back().end > current->GetRanges().back().end) {
        auto loc = spill->GetInstruction()->GetLocation();
        current->GetInstruction()->SetLocation(loc);
        
        stack_offset_ += 8;
        spill->GetInstruction()->SetLocation(Location::MakeStack(-stack_offset_));

        active_.pop_back();
        SortedInsert(active_, current);
    } else {
        stack_offset_ += 8;
        current->GetInstruction()->SetLocation(Location::MakeStack(-stack_offset_));
    }
}

void RegisterAllocator::AllocateRegister(analysis::LiveInterval* current) {
    if (free_regs_.empty()) {
        SpillAtInterval(current);
        return;
    }
    auto reg = *free_regs_.begin();
    free_regs_.erase(free_regs_.begin());
    
    current->GetInstruction()->SetLocation(Location::MakeRegister(reg));
    
    SortedInsert(active_, current);
}

void RegisterAllocator::RewriteAndInsertSpillFill() {
    IRBuilder builder(graph_);
    ResolvePhis(builder);
    SpillFillInstructions(builder);
}

void RegisterAllocator::ResolvePhis(IRBuilder& builder) {
    for (auto& block : graph_->GetBlocks()) {
        auto* terminator = block.GetLastInstruction();
        if (!terminator) continue;

        for (auto* succ : block.GetSuccessors()) {
            if (succ->GetFirstInstruction() == nullptr || succ->GetFirstInstruction()->GetOpcode() != Opcode::PHI) {
                continue;
            }
            
            builder.SetInsertPoint(terminator);
            
            size_t pred_idx = -1;
            const auto& succ_preds = succ->GetPredecessors();
            for (size_t i = 0; i < succ_preds.size(); ++i) {
                if (succ_preds[i] == &block) {
                    pred_idx = i;
                    break;
                }
            }
            if (pred_idx == (size_t)-1) continue;

            struct Move {
                Instruction* to_inst;
                Instruction* from_inst;
            };
            std::list<Move> parallel_moves;
            for (auto* inst = succ->GetFirstInstruction(); inst && inst->GetOpcode() == Opcode::PHI; inst = inst->GetNext()) {
                auto* phi = static_cast<PhiInst*>(inst);
                if (pred_idx < phi->GetInputs().size()) {
                    Instruction* incoming_val = phi->GetInputs()[pred_idx];
                    if (incoming_val) {
                         auto to_loc = phi->GetLocation();
                        auto from_loc = incoming_val->GetLocation();
                        if (to_loc.GetKind() != from_loc.GetKind() || to_loc.GetValue() != from_loc.GetValue()) {
                            parallel_moves.push_back({phi, incoming_val});
                        }
                    }
                }
            }

            if (parallel_moves.empty()) continue;
            
            std::list<Move> pending = parallel_moves;
            std::list<Move> ready;

            while(!pending.empty()) {
                bool found_ready = false;
                for (auto it = pending.begin(); it != pending.end(); ) {
                    bool is_source = false;
                    for (const auto& other : pending) {
                        if (&(*it) != &other && it->to_inst == other.from_inst && it->to_inst->GetLocation().GetKind() == Location::REGISTER) {
                             is_source = true;
                             break;
                        }
                    }
                    if (!is_source) {
                        ready.push_back(*it);
                        it = pending.erase(it);
                        found_ready = true;
                    } else {
                        ++it;
                    }
                }

                if (!found_ready) {
                    auto& move_to_break = pending.front();
                    auto* temp_load = builder.CreateMove(move_to_break.from_inst->GetType(), move_to_break.from_inst);
                    temp_load->SetLocation(Location::MakeRegister(reserved_regs_start_));
                    move_to_break.from_inst = temp_load;
                    continue; 
                }

                for(const auto& move : ready) {
                     auto* new_move = builder.CreateMove(move.from_inst->GetType(), move.from_inst);
                     new_move->SetLocation(move.to_inst->GetLocation());
                }
                ready.clear();
            }
        }
    }
}

void RegisterAllocator::SpillFillInstructions(IRBuilder& builder) {
    std::vector<Instruction*> all_instructions;
    for(auto& block : graph_->GetBlocks()) {
        for(auto* inst = block.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            all_instructions.push_back(inst);
        }
    }

    for (auto* inst : all_instructions) {
         if (inst->GetOpcode() == Opcode::PHI) {
            continue;
        }
        SpillFillForInstruction(inst, builder);
    }
}

void RegisterAllocator::SpillFillForInstruction(Instruction* inst, IRBuilder& builder) {
    std::vector<std::pair<size_t, Instruction*>> spilled_inputs;
    for (size_t i = 0; i < inst->GetInputs().size(); ++i) {
        if (inst->GetInputs()[i] && inst->GetInputs()[i]->GetLocation().GetKind() == Location::STACK) {
            spilled_inputs.emplace_back(i, inst->GetInputs()[i]);
        }
    }

    if (!spilled_inputs.empty()) {
        builder.SetInsertPoint(inst);
        std::unordered_map<Instruction*, Instruction*> reloads;
        uint32_t reserved_idx = 0;
        if (spilled_inputs.size() > num_reserved_regs_) {
            throw std::runtime_error("Not enough reserved registers to spill/fill for one instruction.");
        }

        for (const auto& [index, original_input] : spilled_inputs) {
            Instruction* reloaded_val = nullptr;
            if (reloads.count(original_input)) {
                reloaded_val = reloads.at(original_input);
            } else {
                reloaded_val = builder.CreateLoad(original_input->GetType(), original_input);
                reloaded_val->SetLocation(Location::MakeRegister(reserved_regs_start_ + reserved_idx++));
                reloads[original_input] = reloaded_val;
            }
            inst->SetInput(index, reloaded_val);
        }
    }
    
    if (inst->GetLocation().GetKind() == Location::STACK) {
        auto opcode = inst->GetOpcode();
        if (opcode != Opcode::STORE && opcode != Opcode::JUMP &&
            opcode != Opcode::JA && opcode != Opcode::RET)
        {
            auto* next_inst = inst->GetNext();
            if (next_inst) {
                builder.SetInsertPoint(next_inst);
            } else {
                builder.SetInsertPoint(inst->GetBasicBlock());
            }
            auto* store = builder.CreateStore(inst->GetType(), inst, inst);
            store->SetLocation(inst->GetLocation());
        }
    }
}

} // namespace opt
