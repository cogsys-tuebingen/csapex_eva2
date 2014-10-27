#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// COMPONENT

/// PROJECT
#include <csapex/model/node.h>

/// SYSTEM

namespace csapex {


class EvaOptimizer : public csapex::Node
{
    friend class EvaOptimizerAdapter;

public:
    EvaOptimizer();

    void setupParameters();
    void setup();
    void process();
    void tick();

private:
    void start();
    void stop();

    YAML::Node makeRequest();

private:
    Input* in_;
    Output* out_;

    bool init_;
};


}

#endif // EVA_OPTIMIZER_H
