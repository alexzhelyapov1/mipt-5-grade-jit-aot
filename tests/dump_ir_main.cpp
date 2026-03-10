#include "helpers/factorial_graph.h"
#include "ir/ir.h"
#include <iostream>

int main() {
    Graph graph;
    BuildFactorialGraph(&graph);
    graph.Dump(std::cout);
    return 0;
}
