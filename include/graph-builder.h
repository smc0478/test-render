#ifndef _GRAPH_BUILDER_H_
#define _GRAPH_BUILDER_H_

#include "graph-ir.h"

namespace RD {

class GraphBuilder {
public:
    AddPass();
    AddEdge();
private:
    GraphIr *graph;     // root of graph
};

}

#endif // graph-builder.h