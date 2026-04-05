#pragma once

#include "ir/instruction.h"
#include <map>
#include <vector>
#include <unordered_map>

class Graph;
class CallStaticInst;
class BasicBlock;
class Instruction;

struct ReturnInfo {
    ReturnInst *inst;
    BasicBlock *bb;
};

class Inliner {
  public:
    explicit Inliner(Graph *graph) : graph_(graph) {}
    bool Run();

  private:
    void InlineCall(CallStaticInst *call);
    void MapParameters(Graph *callee, CallStaticInst *call, InstMapping &mapping);
    void HoistConstants(Graph *caller, Graph *callee, InstMapping &mapping);
    void CloneBlocks(Graph *graph, Graph *callee, InstMapping &mapping,
                     std::map<BasicBlock *, BasicBlock *> &bb_map, std::vector<ReturnInfo> &returns,
                     std::unordered_map<PhiInst *, PhiInst *> &phi_map);
    void PatchClonedInstructions(Graph *callee, std::map<BasicBlock *, BasicBlock *> &bb_map,
                                 std::unordered_map<PhiInst *, PhiInst *> &phi_map, InstMapping &mapping,
                                 Graph *caller);
    void ConnectCFG(Graph *callee, BasicBlock *caller_bb, BasicBlock *cont_bb,
                    std::map<BasicBlock *, BasicBlock *> &bb_map, std::vector<ReturnInfo> &returns);
    Instruction *CreateMergePhi(Graph *graph, BasicBlock *cont_bb, CallStaticInst *call,
                                std::vector<ReturnInfo> &returns);

    Graph *graph_;
};
