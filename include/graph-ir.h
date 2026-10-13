#ifndef _GRAPH_IR_H_
#define _GRAPH_IR_H_

#include <vector>
#include "data.h"

namespace RD {


// Declare input data in graph
// do 
#define DECL_GRAPH_IN(type, name) \
    RData<type> name;

#define DECL_GRAPH_OUT(type, name) \
    RData<type> name;

class GraphIr {
public:
    
    

private:

    template <typename T>
    T* Get(int idx);
    
    template <typename T>
    Set(T* data);

    static int _id = 0;
    int id;
    std::vector<GraphIr*> in;       // connected input ir
    std::vector<GraphIr*> out;      // connected output ir
};

}

#endif // graph-ir.h