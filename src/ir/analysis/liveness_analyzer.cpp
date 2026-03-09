#include "ir/analysis/liveness_analyzer.h"
#include "ir/basic_block.h"
#include "ir/graph.h"
#include "ir/instruction.h"
#include <algorithm>
#include <iostream>
#include <vector>

namespace analysis {

LivenessAnalyzer::LivenessAnalyzer(Graph *graph) : graph_(graph), linear_order_(graph), loop_analyzer_(graph) {}

LivenessAnalyzer::~LivenessAnalyzer() {
  for (auto const &[inst, interval] : intervals_) {
    delete interval;
  }
}

void LivenessAnalyzer::NumberInstructions() {
  uint32_t current_pos = 0;
  const auto &blocks = linear_order_.GetBlocks();
  for (BasicBlock *bb : blocks) {
    if (current_pos % 2 != 0)
      current_pos++;
    uint32_t from = current_pos;

    for (Instruction *inst = bb->GetFirstInstruction(); inst != nullptr; inst = inst->GetNext()) {
      inst_positions_[inst] = current_pos;
      current_pos += 2;
    }
    uint32_t to = current_pos;
    block_positions_[bb] = {from, to};
  }
}

LiveInterval *LivenessAnalyzer::GetOrCreateInterval(const Instruction *inst) {
  auto it = intervals_.find(inst);
  if (it != intervals_.end()) {
    return it->second;
  }
  auto *new_interval = new LiveInterval(inst);
  intervals_[inst] = new_interval;
  return new_interval;
}

void LivenessAnalyzer::Analyze() {
  loop_analyzer_.Analyze();
  NumberInstructions();

  std::unordered_map<BasicBlock *, std::unordered_set<const Instruction *>> live_in_sets;
  const auto &blocks = linear_order_.GetBlocks();

  for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
    ProcessBlock(*it, live_in_sets);
  }
}

void LivenessAnalyzer::ProcessBlock(
    BasicBlock *block, std::unordered_map<BasicBlock *, std::unordered_set<const Instruction *>> &live_in_sets) {
  std::unordered_set<const Instruction *> live;

  for (auto *succ : block->GetSuccessors()) {
    if (live_in_sets.count(succ)) {
      live.insert(live_in_sets.at(succ).begin(), live_in_sets.at(succ).end());
    }
  }

  uint32_t block_from = block_positions_.at(block).start;
  uint32_t block_to = block_positions_.at(block).end;

  for (auto *succ : block->GetSuccessors()) {
    for (auto *inst = succ->GetFirstInstruction(); inst && inst->GetOpcode() == Opcode::PHI; inst = inst->GetNext()) {
      auto *phi = static_cast<PhiInst *>(inst);
      auto &preds = succ->GetPredecessors();
      for (size_t i = 0; i < preds.size(); ++i) {
        if (preds[i] == block) {
          if (i < phi->GetInputs().size() && phi->GetInputs()[i] != nullptr) {
            live.insert(phi->GetInputs()[i]);
          }
        }
      }
    }
  }

  for (auto *inst : live) {
    GetOrCreateInterval(inst)->AddRange(block_from, block_to);
  }

  for (auto *inst = block->GetLastInstruction(); inst != nullptr; inst = inst->GetPrev()) {
    uint32_t inst_pos = inst_positions_.at(inst);

    if (inst->GetOpcode() != Opcode::PHI && inst->GetType() != Type::VOID && !dynamic_cast<TerminatorInst *>(inst)) {
      GetOrCreateInterval(inst)->SetStart(inst_pos);
      live.erase(inst);
    }

    if (inst->GetOpcode() != Opcode::PHI) {
      for (auto *input : inst->GetInputs()) {
        if (input == nullptr)
          continue;
        GetOrCreateInterval(input)->AddRange(block_from, inst_pos);
        live.insert(input);
      }
    }
  }

  for (auto *inst = block->GetFirstInstruction(); inst && inst->GetOpcode() == Opcode::PHI; inst = inst->GetNext()) {
    GetOrCreateInterval(inst)->SetStart(block_from);
    live.erase(inst);
  }

  if (loop_analyzer_.IsLoopHeader(block)) {
    Loop *loop = loop_analyzer_.GetLoopForBlock(block);
    if (loop) {
      uint32_t loop_end = block_positions_.at(block).end;
      for (auto *loop_block : loop->GetBlocks()) {
        if (block_positions_.count(loop_block)) {
          loop_end = std::max(loop_end, block_positions_.at(loop_block).end);
        }
      }

      for (auto *inst : live) {
        GetOrCreateInterval(inst)->AddRange(block_from, loop_end);
      }
      for (auto *inst = block->GetFirstInstruction(); inst && inst->GetOpcode() == Opcode::PHI;
           inst = inst->GetNext()) {
        GetOrCreateInterval(inst)->AddRange(block_from, loop_end);
      }
    }
  }

  live_in_sets[block] = std::move(live);
}

uint32_t LivenessAnalyzer::GetInstructionPosition(const Instruction *inst) const {
  auto it = inst_positions_.find(inst);
  if (it != inst_positions_.end()) {
    return it->second;
  }
  return 0;
}

LiveInterval *LivenessAnalyzer::GetLiveInterval(const Instruction *inst) const {
  auto it = intervals_.find(inst);
  if (it != intervals_.end()) {
    return it->second;
  }
  return nullptr;
}

const std::vector<BasicBlock *> &LivenessAnalyzer::GetLinearOrder() const { return linear_order_.GetBlocks(); }

void LivenessAnalyzer::Dump(std::ostream &os) const {
  os << "Liveness Analysis Results:\n";
  os << "==========================\n";
  os << "Linear Order Blocks: ";
  for (const auto *bb : GetLinearOrder()) {
    os << "BB" << bb->GetId() << " ";
  }
  os << "\n\n";

  os << "Instruction Positions:\n";
  std::vector<const Instruction *> ordered_insts;
  ordered_insts.reserve(inst_positions_.size());
  for (auto const &[inst, pos] : inst_positions_) {
    ordered_insts.push_back(inst);
  }
  std::sort(ordered_insts.begin(), ordered_insts.end(), [this](const Instruction *a, const Instruction *b) {
    return inst_positions_.at(a) < inst_positions_.at(b);
  });
  for (const auto *inst : ordered_insts) {
    os << "  " << inst_positions_.at(inst) << ": i" << inst->GetId() << "  (";
    inst->Print(os);
    os << ")\n";
  }
  os << "\n";

  os << "Live Intervals:\n";
  for (const auto *inst : ordered_insts) {
    auto it = intervals_.find(inst);
    if (it != intervals_.end()) {
      it->second->Dump(os);
      os << "\n";
    }
  }
}

} // namespace analysis
