/// HEADER
#include "eva_optimizer.h"

/// PROJECT
#include <csapex/msg/input.h>
#include <csapex/msg/output.h>
#include <csapex/utility/register_apex_plugin.h>
#include <utils_param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <csapex/model/node_worker.h>
#include <utils_param/range_parameter.h>

/// SYSTEM
//#include <boost/assign.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaOptimizer, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;


EvaOptimizer::EvaOptimizer()
    : init_(false)
{
}

void EvaOptimizer::setupParameters()
{
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
    // initilization?
    if(!init_) {
        // TODO: connect to eva

        // generate request
        YAML::Node description = makeRequest();
        ainfo << "optimization request:\n" << description << std::endl;

        // TODO: send parameter description

        init_ = true;
    }
    // TODO: wait for set of parameters
    // TODO: set parameter values to the values specified by eva

    // DUMMY
    foreach(param::Parameter::Ptr p, getParameters()) {
        param::RangeParameter::Ptr dbl_range = boost::dynamic_pointer_cast<param::RangeParameter>(p);
        if(!dbl_range || !dbl_range->is<double>()) {
            continue;
        }
        dbl_range->set<double>(dbl_range->as<double>() + 4*dbl_range->step<double>());
        break;
    }

    // start another evaluation
    AnyMessage::Ptr trigger(new AnyMessage);
    out_->publish(trigger);
}

void EvaOptimizer::process()
{
    double fitness = in_->getValue<double>();
    ainfo << "got fitness: " << fitness << std::endl;

    // TODO: send fitness back to eva
}

YAML::Node EvaOptimizer::makeRequest()
{
    YAML::Node req;
    // TODO: make parameter
    // TODO: define which are possible
    req["method"] = "HillClimbing";

    foreach(param::Parameter::Ptr p, getParameters()) {
        // TODO: support more than double ranges
        param::RangeParameter::Ptr dbl_range = boost::dynamic_pointer_cast<param::RangeParameter>(p);
        if(!dbl_range || !dbl_range->is<double>()) {
            continue;
        }

        YAML::Node param_node;
        param_node["name"] = p->name();
        param_node["type"] = "double/range";
        param_node["min"] = dbl_range->min<double>();
        param_node["max"] = dbl_range->max<double>();
        param_node["step"] = dbl_range->step<double>();

        req["params"].push_back(param_node);
    }
    return req;
}

void EvaOptimizer::start()
{
    ainfo << "starting optimization" << std::endl;
    init_ = false;
    getNodeWorker()->setTickEnabled(true);
}


void EvaOptimizer::stop()
{
    ainfo << "stopping optimization" << std::endl;
    getNodeWorker()->setTickEnabled(false);
}
