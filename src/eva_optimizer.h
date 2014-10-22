#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// COMPONENT

/// PROJECT
#include <csapex/model/node.h>

/// SYSTEM

namespace csapex {


class EvaOptimizer : public csapex::Node
{
public:
    EvaOptimizer();

    void setupParameters();
    void setup();
    void process();
    void tick();

private:
    void start();

private:
    Input* in_;
    Output* out_;
};


}

#endif // EVA_OPTIMIZER_H
