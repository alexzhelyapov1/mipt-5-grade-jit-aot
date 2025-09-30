#include "ir/ir.h"
#include <iostream>
#include "helpers/factorial_graph.h"

int main() {
    Graph graph;
    BuildFactorialGraph(&graph);
    graph.Dump(std::cout);
    return 0;
}
