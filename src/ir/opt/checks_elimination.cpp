#include "ir/opt/checks_elimination.h"
#include "ir/analysis/graph_analyzer.h"
#include "ir/basic_block.h"
#include "ir/instruction.h"
#include "ir/ir_builder.h"
#include <unordered_set>
#include <vector>

namespace opt {

void ChecksElimination::Run() {
    EliminateDominatedChecks();
    EliminateRedundantBoundsChecks();
    EliminateMustThrowChecks();
}

void ChecksElimination::EliminateMustThrowChecks() {
    std::vector<Instruction *> must_throw;

    for (BasicBlock &bb : graph_->GetBlocks()) {
        for (Instruction *inst = bb.GetFirstInstruction(); inst != nullptr; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::NULL_CHECK) {
                Instruction *obj = inst->GetInputs()[0];
                if (obj->GetOpcode() == Opcode::Constant) {

                    uint64_t val = static_cast<ConstantInst *>(obj)->GetValue();
                    if (val == 0) {
                        must_throw.push_back(inst);
                    }
                }
            } else if (inst->GetOpcode() == Opcode::BOUNDS_CHECK) {
                Instruction *index_inst = inst->GetInputs()[0];
                Instruction *len_inst = inst->GetInputs()[1];
                if (index_inst->GetOpcode() == Opcode::Constant && len_inst->GetOpcode() == Opcode::Constant) {

                    int64_t index = static_cast<int64_t>(static_cast<ConstantInst *>(index_inst)->GetValue());
                    uint64_t len = static_cast<ConstantInst *>(len_inst)->GetValue();
                    if (index < 0 || (uint64_t)index >= len) {
                        must_throw.push_back(inst);
                    }
                }
            }
        }
    }

    for (Instruction *inst : must_throw) {
        BasicBlock *bb = inst->GetBasicBlock();
        if (bb == nullptr)
            continue;

        IRBuilder builder(graph_);
        builder.SetInsertPoint(inst);
        builder.CreateDeoptimize();

        Instruction *curr = inst;
        while (curr != nullptr) {
            Instruction *next = curr->GetNext();
            bb->RemoveInstruction(curr);
            curr = next;
        }

        bb->ClearSuccessors();
    }
}

void ChecksElimination::EliminateRedundantBoundsChecks() {
    LoopAnalyzer loop_analyzer(graph_);
    loop_analyzer.Analyze();

    analysis::BoundsAnalysis bounds_analysis(graph_);
    bounds_analysis.Run(&loop_analyzer);

    std::unordered_set<Instruction *> to_remove;

    for (BasicBlock &bb : graph_->GetBlocks()) {
        Loop *loop = loop_analyzer.GetLoopForBlock(&bb);
        if (loop == nullptr || !loop->IsCountable()) {
            continue;
        }

        const auto *bounds = bounds_analysis.GetLoopBounds(loop);
        if (bounds == nullptr || !bounds->is_countable) {
            continue;
        }

        for (Instruction *inst = bb.GetFirstInstruction(); inst != nullptr; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::BOUNDS_CHECK) {
                Instruction *index = inst->GetInputs()[0];
                Instruction *len_inst = inst->GetInputs()[1];

                if (index->GetOpcode() == Opcode::PHI && index->GetBasicBlock() == loop->GetHeader()) {
                    uint64_t init = 0;
                    bool init_is_const = false;
                    if (bounds->init->GetOpcode() == Opcode::Constant) {
                        init = static_cast<ConstantInst *>(bounds->init)->GetValue();
                        init_is_const = true;
                    }

                    bool test_is_len = (bounds->test == len_inst);
                    bool test_is_const = (bounds->test->GetOpcode() == Opcode::Constant);
                    uint64_t test_val = test_is_const ? static_cast<ConstantInst *>(bounds->test)->GetValue() : 0;
                    uint64_t len_val = (len_inst->GetOpcode() == Opcode::Constant)
                                           ? static_cast<ConstantInst *>(len_inst)->GetValue()
                                           : 0;
                    bool len_is_const = (len_inst->GetOpcode() == Opcode::Constant);

                    uint64_t step = 0;
                    if (bounds->step->GetOpcode() == Opcode::Constant) {
                        step = static_cast<ConstantInst *>(bounds->step)->GetValue();
                    }

                    bool always_in_bounds = false;
                    if (bounds->cc == ConditionCode::LT && step > 0) {
                        if (init_is_const && (int64_t)init >= 0) {
                            if (test_is_len) {
                                always_in_bounds = true;
                            } else if (test_is_const && len_is_const && test_val <= len_val) {
                                always_in_bounds = true;
                            }
                        }
                    } else if (bounds->cc == ConditionCode::LE && step > 0) {
                        if (init_is_const && (int64_t)init >= 0) {
                            if (test_is_const && len_is_const && test_val < len_val) {
                                always_in_bounds = true;
                            }
                        }
                    }

                    if (always_in_bounds) {
                        to_remove.insert(inst);
                    }
                }
            }
        }
    }

    for (Instruction *inst : to_remove) {
        if (inst->GetBasicBlock() != nullptr) {
            inst->GetBasicBlock()->RemoveInstruction(inst);
        }
    }
}

void ChecksElimination::EliminateDominatedChecks() {
    GraphAnalyzer analyzer(graph_);
    analyzer.BuildDominatorTree();

    const auto &rpo = analyzer.GetReversePostOrder();

    std::unordered_set<Instruction *> to_remove;

    for (BasicBlock *bb : rpo) {
        for (Instruction *inst = bb->GetFirstInstruction(); inst != nullptr; inst = inst->GetNext()) {
            if (to_remove.count(inst)) {
                continue;
            }

            if (inst->GetOpcode() == Opcode::NULL_CHECK) {
                Instruction *obj = inst->GetInputs()[0];
                for (User *u = obj->GetFirstUser(); u != nullptr; u = u->GetNextUser()) {
                    Instruction *user_inst = u->GetUserInstruction();
                    if (user_inst == inst || to_remove.count(user_inst))
                        continue;

                    Opcode user_op = user_inst->GetOpcode();
                    if (user_op == Opcode::NULL_CHECK || user_op == Opcode::LOAD || user_op == Opcode::STORE) {
                        if (Dominates(inst, user_inst, analyzer)) {
                            if (user_op == Opcode::NULL_CHECK) {
                                to_remove.insert(user_inst);
                            }
                        } else if (Dominates(user_inst, inst, analyzer)) {
                            to_remove.insert(inst);
                            break;
                        }
                    }
                }
            } else if (inst->GetOpcode() == Opcode::BOUNDS_CHECK) {
                Instruction *index = inst->GetInputs()[0];
                Instruction *len = inst->GetInputs()[1];
                for (User *u = index->GetFirstUser(); u != nullptr; u = u->GetNextUser()) {
                    Instruction *user_inst = u->GetUserInstruction();

                    if (user_inst != inst && user_inst->GetOpcode() == Opcode::BOUNDS_CHECK &&
                        !to_remove.count(user_inst)) {

                        if (user_inst->GetInputs()[0] == index && user_inst->GetInputs()[1] == len) {
                            if (Dominates(inst, user_inst, analyzer)) {
                                to_remove.insert(user_inst);
                            }
                        }
                    }
                }
            }
        }
    }

    for (Instruction *inst : to_remove) {
        if (inst->GetBasicBlock() != nullptr) {
            inst->GetBasicBlock()->RemoveInstruction(inst);
        }
    }
}

bool ChecksElimination::Dominates(Instruction *dom, Instruction *inst, const GraphAnalyzer &analyzer) {
    BasicBlock *dom_bb = dom->GetBasicBlock();
    BasicBlock *inst_bb = inst->GetBasicBlock();

    if (dom_bb != inst_bb) {
        return analyzer.Dominates(dom_bb, inst_bb);
    }

    for (Instruction *curr = dom->GetNext(); curr != nullptr; curr = curr->GetNext()) {
        if (curr == inst) {
            return true;
        }
    }

    return false;
}

} // namespace opt
