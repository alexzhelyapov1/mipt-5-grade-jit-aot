#include "ir/opt/peephole_optimizer.h"
#include "ir/basic_block.h"
#include "ir/graph.h"
#include "ir/instruction.h"
#include "ir/ir_builder.h"

PeepholeOptimizer::PeepholeOptimizer(Graph *graph) : graph_(graph) {}

void PeepholeOptimizer::Run() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto &bb : graph_->GetBlocks()) {
      for (auto *inst = bb.GetFirstInstruction(); inst != nullptr; inst = inst->GetNext()) {
        if (inst->GetFirstUser() == nullptr && inst->GetType() != Type::VOID) {
          continue;
        }

        Instruction *replacement = TryFold(inst);
        if (replacement && replacement != inst) {
          inst->ReplaceAllUsesWith(replacement);
          changed = true;
        }
      }
    }
  }
}

static ConstantInst *AsConstant(Instruction *inst) { return dynamic_cast<ConstantInst *>(inst); }

static bool IsNegationOf(Instruction *inst, Instruction *target) {
  if (inst->GetOpcode() != Opcode::MUL) {
    return false;
  }

  auto *lhs = inst->GetInputs()[0];
  auto *rhs = inst->GetInputs()[1];
  auto *c_lhs = dynamic_cast<ConstantInst *>(lhs);
  auto *c_rhs = dynamic_cast<ConstantInst *>(rhs);

  if (lhs == target && c_rhs && c_rhs->GetValue() == static_cast<uint64_t>(-1)) {
    return true;
  }
  if (rhs == target && c_lhs && c_lhs->GetValue() == static_cast<uint64_t>(-1)) {
    return true;
  }

  return false;
}

Instruction *PeepholeOptimizer::TryFold(Instruction *inst) {
  IRBuilder builder(graph_);
  if (inst->GetBasicBlock()) {
    builder.SetInsertPoint(inst->GetBasicBlock());
  }

  if (auto *bin_op = dynamic_cast<BinaryInst *>(inst)) {
    auto *lhs = bin_op->GetInputs()[0];
    auto *rhs = bin_op->GetInputs()[1];
    auto *lc = AsConstant(lhs);
    auto *rc = AsConstant(rhs);

    switch (inst->GetOpcode()) {
    case Opcode::ADD:
      // Constant Folding
      if (lc && rc) {
        return builder.CreateConstant(inst->GetType(), lc->GetValue() + rc->GetValue());
      }
      // x + 0 -> x
      if (rc && rc->GetValue() == 0) {
        return lhs;
      }
      if (lc && lc->GetValue() == 0) {
        return rhs;
      }
      // x + x -> x << 1
      if (lhs == rhs) {
        auto *one = builder.CreateConstant(Type::U32, 1);
        return builder.CreateShl(lhs, one);
      }
      // x + (-x) -> 0
      if (IsNegationOf(rhs, lhs) || IsNegationOf(lhs, rhs)) {
        return builder.CreateConstant(inst->GetType(), 0);
      }
      break;

    case Opcode::AND:
      if (lc && rc)
        return builder.CreateConstant(inst->GetType(), lc->GetValue() & rc->GetValue());
      if (rc && rc->GetValue() == 0)
        return rc;
      if (lc && lc->GetValue() == 0)
        return lc;
      if (lhs == rhs)
        return lhs;
      if (rc && rc->GetValue() == static_cast<uint64_t>(-1))
        return lhs;
      break;

    case Opcode::SHL:
      if (lc && rc)
        return builder.CreateConstant(inst->GetType(), lc->GetValue() << rc->GetValue());
      if (rc && rc->GetValue() == 0)
        return lhs;
      if (lc && lc->GetValue() == 0)
        return lc;
      break;

    default:
      break;
    }
  }
  return nullptr;
}
