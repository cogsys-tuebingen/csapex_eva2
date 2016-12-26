/// HEADER
#include "eva_optimizer_adapter.h"

/// PROJECT
#include <csapex/view/utility/register_node_adapter.h>

using namespace csapex;

CSAPEX_REGISTER_NODE_ADAPTER(EvaOptimizerAdapter, csapex::EvaOptimizer)


EvaOptimizerAdapter::EvaOptimizerAdapter(NodeHandleWeakPtr worker, NodeBox* parent, std::weak_ptr<EvaOptimizer> node)
    : OptimizerAdapter(worker, parent, node), wrapped_(node)
{
}

EvaOptimizerAdapter::~EvaOptimizerAdapter()
{
}
