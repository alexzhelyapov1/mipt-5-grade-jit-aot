#include "ir/opt/inliner.h"
#include "ir/basic_block.h"
#include "ir/graph.h"
#include "ir/instruction.h"
#include <algorithm>
#include <unordered_map>

void Inliner::MapParameters(Graph *callee, CallStaticInst *call, InstMapping &mapping) {
    auto &callee_args = callee->GetArguments();
    auto &caller_args = call->GetInputs();
    for (size_t i = 0; i < callee_args.size(); ++i) {
        mapping[callee_args[i]] = caller_args[i];
    }
}

void Inliner::HoistConstants(Graph *caller, Graph *callee, InstMapping &mapping) {
    auto *start_bb = caller->GetStartBlock();
    for (auto &bb : callee->GetBlocks()) {
        for (auto *inst = bb.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() != Opcode::Constant || mapping.count(inst)) {
                continue;
            }
            auto *c = static_cast<ConstantInst *>(inst);
            auto *new_c = new ConstantInst(caller->next_inst_id_++, c->GetType(), c->GetValue());
            caller->instructions_.push_back(std::unique_ptr<Instruction>(new_c));
            if (start_bb->GetFirstInstruction()) {
                start_bb->InsertBefore(new_c, start_bb->GetFirstInstruction());
            } else {
                start_bb->PushBackInstruction(new_c);
            }
            mapping[inst] = new_c;
        }
    }
}

void Inliner::CloneBlocks(Graph *graph, Graph *callee, InstMapping &mapping,
                        std::map<BasicBlock *, BasicBlock *> &bb_map, std::vector<ReturnInfo> &returns,
                        std::unordered_map<PhiInst *, PhiInst *> &phi_map) {
    for (auto &bb : callee->GetBlocks()) {
        auto *new_bb = graph->CreateBasicBlock();
        bb_map[&bb] = new_bb;
        for (auto *inst = bb.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::Argument || inst->GetOpcode() == Opcode::Constant) {
                continue;
            }
            auto *new_inst = inst->Clone(graph, mapping);
            new_inst->SetId(graph->next_inst_id_++);
            graph->instructions_.push_back(std::unique_ptr<Instruction>(new_inst));
            new_bb->PushBackInstruction(new_inst);
            mapping[inst] = new_inst;
            if (new_inst->GetOpcode() == Opcode::RET) {
                returns.push_back({static_cast<ReturnInst *>(new_inst), new_bb});
            } else if (new_inst->GetOpcode() == Opcode::PHI) {
                phi_map[static_cast<PhiInst *>(new_inst)] = static_cast<PhiInst *>(inst);
            }
        }
    }
}

void Inliner::PatchClonedInstructions(Graph *callee, std::map<BasicBlock *, BasicBlock *> &bb_map,
                                    std::unordered_map<PhiInst *, PhiInst *> &phi_map, InstMapping &mapping,
                                    Graph *caller) {
    for (auto &bb : callee->GetBlocks()) {
        auto *new_bb = bb_map[&bb];
        for (auto *inst = new_bb->GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::JUMP) {
                auto *jump = static_cast<JumpInst *>(inst);
                jump->SetTarget(bb_map[jump->GetTarget()]);
            } else if (inst->GetOpcode() == Opcode::JA) {
                auto *branch = static_cast<BranchInst *>(inst);
                branch->SetTrueBB(bb_map[branch->GetTrueBB()]);
                branch->SetFalseBB(bb_map[branch->GetFalseBB()]);
            } else if (inst->GetOpcode() == Opcode::PHI) {
                auto *phi = static_cast<PhiInst *>(inst);
                auto *old_phi = phi_map[phi];
                phi->ClearInputs();
                for (size_t i = 0; i < old_phi->GetInputs().size(); ++i) {
                    phi->AddInput(MapInput(old_phi->GetInputs()[i], mapping));
                    caller->RegisterUse(phi->GetInputs().back(), phi, static_cast<uint32_t>(i));
                }
            }
        }
    }
}

void Inliner::ConnectCFG(Graph *callee, BasicBlock *caller_bb, BasicBlock *cont_bb,
                       std::map<BasicBlock *, BasicBlock *> &bb_map, std::vector<ReturnInfo> &returns) {
    auto *first_bb = bb_map[callee->GetStartBlock()];
    // разрезал все ок
    caller_bb->AddSuccessor(first_bb);
    first_bb->AddPredecessor(caller_bb);
    for (auto &bb : callee->GetBlocks()) {
        auto *new_bb = bb_map[&bb];
        for (auto *succ : bb.GetSuccessors()) {
            auto *new_succ = bb_map[succ];
            new_bb->AddSuccessor(new_succ);
            new_succ->AddPredecessor(new_bb);
        }
    }
    if (returns.empty() && callee->GetBlocks().empty()) {
        // TODO: еще один саксессор под условиями, если условие может быть тру, как прыгнуть по этой дуге? Это условие недостижимо?
        // эксепшн в коцне - континюатион блок недостижим
        caller_bb->AddSuccessor(cont_bb);
        cont_bb->AddPredecessor(caller_bb);
    }
}

Instruction *Inliner::CreateMergePhi(Graph *graph, BasicBlock *cont_bb, CallStaticInst *call,
                                   std::vector<ReturnInfo> &returns) {
    if (returns.size() <= 1) {
        if (returns.size() == 1) {
            returns[0].bb->AddSuccessor(cont_bb);
            cont_bb->AddPredecessor(returns[0].bb);
            Instruction *val = returns[0].inst->GetInputs().empty() ? nullptr : returns[0].inst->GetInputs()[0];
            returns[0].bb->RemoveInstruction(returns[0].inst);
            return val;
        }
        return nullptr;
    }
    auto *phi = new PhiInst(graph->next_inst_id_++, call->GetType());
    graph->instructions_.push_back(std::unique_ptr<Instruction>(phi));
    cont_bb->InsertBefore(phi, cont_bb->GetFirstInstruction());
    phi->SetBasicBlock(cont_bb);
    for (auto &ret : returns) {
        ret.bb->AddSuccessor(cont_bb);
        cont_bb->AddPredecessor(ret.bb);
        if (!ret.inst->GetInputs().empty()) {
            phi->AddIncoming(ret.inst->GetInputs()[0], ret.bb);
        }
        ret.bb->RemoveInstruction(ret.inst);
    }
    return phi;
}

void Inliner::InlineCall(CallStaticInst *call) {
    auto *caller_bb = call->GetBasicBlock();
    // TODO fix + assert
    auto *callee = call->GetCallee();
    auto *cont_bb = caller_bb->SplitAt(call->GetNext());
    caller_bb->RemoveInstruction(call);

    InstMapping mapping;
    MapParameters(callee, call, mapping);
    HoistConstants(graph_, callee, mapping);

    std::map<BasicBlock *, BasicBlock *> bb_map;
    std::vector<ReturnInfo> returns;
    std::unordered_map<PhiInst *, PhiInst *> phi_map;
    CloneBlocks(graph_, callee, mapping, bb_map, returns, phi_map);
    PatchClonedInstructions(callee, bb_map, phi_map, mapping, graph_);
    ConnectCFG(callee, caller_bb, cont_bb, bb_map, returns);

    Instruction *res = CreateMergePhi(graph_, cont_bb, call, returns);
    if (res) {
        call->ReplaceAllUsesWith(res);
    }
}

bool Inliner::Run() {
    bool changed = false;
    std::vector<CallStaticInst *> calls;
    for (auto &bb : graph_->GetBlocks()) {
        for (auto *inst = bb.GetFirstInstruction(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::CALL_STATIC) {
                calls.push_back(static_cast<CallStaticInst *>(inst));
            }
        }
    }
    for (auto *call : calls) {
        InlineCall(call);
        changed = true;
    }
    return changed;
}
