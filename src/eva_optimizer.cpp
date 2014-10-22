/// HEADER
#include "eva_optimizer.h"

/// PROJECT
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/utility/register_apex_plugin.h>
#include <utils_param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <csapex/model/node_worker.h>

/// SYSTEM
//#include <boost/assign.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaOptimizer, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;


EvaOptimizer::EvaOptimizer()
{
}

void EvaOptimizer::setupParameters()
{
    addParameter(param::ParameterFactory::declareTrigger("start optimization"),
                 boost::bind(&EvaOptimizer::start, this));
}

void EvaOptimizer::setup()
{
    in_  = modifier_->addInput<double>("Fitness");
    out_ = modifier_->addOutput<AnyMessage>("Trigger");


//    getNodeWorker()->setIsSource(true);
    getNodeWorker()->setIsSink(true);
}

void EvaOptimizer::tick()
{
    getNodeWorker()->setTickEnabled(false);

    ainfo << "starting optimization" << std::endl;
    AnyMessage::Ptr trigger(new AnyMessage);
    out_->publish(trigger);
}

void EvaOptimizer::process()
{
    double fitness = in_->getValue<double>();
    ainfo << "got fitness: " << fitness << std::endl;
}

void EvaOptimizer::start()
{
    getNodeWorker()->setTickEnabled(true);
}
