#pragma once

class Graph;
class Instruction;

class PeepholeOptimizer {
public:
  explicit PeepholeOptimizer(Graph *graph);
  void Run();

private:
  Instruction *TryFold(Instruction *inst);

  Graph *graph_;
};
