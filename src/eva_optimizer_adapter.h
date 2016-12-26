#ifndef EVA_OPTIMIZER_ADAPTER_H
#define EVA_OPTIMIZER_ADAPTER_H

/// PROJECT
#include <csapex_optimization/optimizer_adapter.h>

/// COMPONENT
#include "eva_optimizer.h"

class QDialog;

namespace csapex {

class EvaOptimizerAdapter : public OptimizerAdapter
{
    Q_OBJECT

public:
    EvaOptimizerAdapter(NodeHandleWeakPtr worker, NodeBox* parent, std::weak_ptr<EvaOptimizer> node);
    ~EvaOptimizerAdapter();


protected:
    std::weak_ptr<EvaOptimizer> wrapped_;
};

}
#endif // EVA_OPTIMIZER_ADAPTER_H
