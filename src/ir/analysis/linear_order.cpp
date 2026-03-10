#include "ir/analysis/linear_order.h"
#include "ir/analysis/graph_analyzer.h"
#include "ir/graph.h"

namespace analysis {

LinearOrder::LinearOrder(Graph *graph) : graph_(graph), graph_analyzer_(graph) { Build(); }

void LinearOrder::Build() {
    graph_analyzer_.ComputeRPO();
    linear_order_ = graph_analyzer_.GetReversePostOrder();
}

} // namespace analysis
