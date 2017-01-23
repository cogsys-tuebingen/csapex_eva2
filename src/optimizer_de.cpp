#include "optimizer_de.h"

#include <csapex/param/range_parameter.h>
#include <csapex/param/interval_parameter.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/param/output_progress_parameter.h>

using namespace csapex;
using namespace cslibs_jcppsocket;

OptimizerDE::OptimizerDE()
{

}

std::string OptimizerDE::getName() const
{
    return "DE";
}


void OptimizerDE::getOptions(YAML::Node &options)
{
    AbstractOptimizer::getOptions(options);

    options["individuals/later_generations"] = individuals_later_;
}


void OptimizerDE::addParameters(Parameterizable& params)
{
    AbstractOptimizer::addParameters(params);

    params.addTemporaryParameter(param::ParameterFactory::declareRange("individuals/target", 30, 30, 30, 1),
                                 individuals_later_);

    params.addTemporaryParameter(csapex::param::ParameterFactory::declareRange("generations", -1, 1024, -1, 1), [this](param::Parameter* p) {
        generations_ = p->as<int>();
        if(generations_ == -1) {
            progress_generation_->setProgress(0, 0);
        } else {
            progress_generation_->setProgress(generation_, generations_);
        }
    });

    param::Parameter::Ptr pg = csapex::param::ParameterFactory::declareOutputProgress("generation");
    progress_generation_ = dynamic_cast<param::OutputProgressParameter*>(pg.get());
    params.addTemporaryParameter(pg);
}

bool OptimizerDE::canContinue() const
{
    return generations_ == -1 || generation_ < generations_;
}

void OptimizerDE::nextIteration()
{
    if(generations_ == -1) {
        progress_generation_->setProgress(0, 0);
    } else {
        ++generation_;
        progress_generation_->setProgress(generation_, generations_);
    }

    AbstractOptimizer::nextIteration();
}

void OptimizerDE::terminate()
{
    progress_generation_->setProgress(generations_, generations_);
}

void OptimizerDE::encodeParameters(const std::vector<param::ParameterPtr>& params, YAML::Node &out)
{
    for(csapex::param::Parameter::Ptr p : params) {
        param::RangeParameter::Ptr range = std::dynamic_pointer_cast<param::RangeParameter>(p);
        if(range) {
            if(range->is<double>()) {
                YAML::Node param_node;
                param_node["name"] = p->name();
                param_node["type"] = "double/range";
                param_node["min"] = range->min<double>();
                param_node["max"] = range->max<double>();
                param_node["step"] = range->step<double>();

                out["params"].push_back(param_node);
                continue;

            } else if(range->is<int>()) {
                YAML::Node param_node;
                param_node["name"] = p->name();
                param_node["type"] = "double/range";
                param_node["min"] = (double) range->min<int>();
                param_node["max"] = (double)range->max<int>();
                param_node["step"] = (double)range->step<int>();

                out["params"].push_back(param_node);
                continue;
            }
        }

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval && interval->is<std::pair<int, int>>()) {
            YAML::Node node_low;
            node_low["name"] = p->name();
            node_low["type"] = "double/range";
            node_low["min"] = (double) interval->min<int>();
            node_low["max"] = (double) interval->max<int>();
            node_low["step"] = (double) interval->step<int>();

            YAML::Node node_high;
            node_high["name"] = p->name();
            node_high["type"] = "double/range";
            node_high["min"] = (double) interval->min<int>();
            node_high["max"] = (double) interval->max<int>();
            node_high["step"] = (double) interval->step<int>();

            out["params"].push_back(node_low);
            out["params"].push_back(node_high);
            continue;
        }
    }
}

void OptimizerDE::decodeParameters(const cslibs_jcppsocket::SocketMsg::Ptr &msg,
                                   const std::vector<param::ParameterPtr>& params)
{
    // read a set of parameters
    VectorMsg<double>::Ptr values = std::dynamic_pointer_cast<VectorMsg<double> >(msg);
    if(!values) {
        throw std::runtime_error("didn't get parameters");
    }

    current_parameter_set_  = values;

    if(!current_parameter_set_) {
        return;
    }

    std::vector<csapex::param::Parameter::Ptr> supported_params;

    for(csapex::param::Parameter::Ptr p : params) {
        // TODO: support more types
        param::RangeParameter::Ptr range = std::dynamic_pointer_cast<param::RangeParameter>(p);
        if(range) {
            if(range->is<double>()) {
                supported_params.push_back(p);

            } else if(range->is<int>()) {
                supported_params.push_back(p);
            }
        }

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval && interval->is<std::pair<int, int>>()) {
            supported_params.push_back(p);
            supported_params.push_back(p);
        }
    }

    if(current_parameter_set_->size() != supported_params.size()) {
        std::stringstream msg;
        msg << "number of parameters is wrong: " << current_parameter_set_->size() <<
               " vs. " << supported_params.size() << std::endl;
        throw std::runtime_error(msg.str());
    }

    // set parameter values to the values specified by eva
    for(std::size_t i = 0; i < supported_params.size(); ++i) {
        csapex::param::Parameter::Ptr p = supported_params[i];

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval) {
            auto low = interval;
            auto high = std::dynamic_pointer_cast<param::IntervalParameter>(supported_params[i]);
            apex_assert(low);
            apex_assert(high);
            p->set<std::pair<int, int>>(std::pair<int, int>
                                        (current_parameter_set_->at(i), current_parameter_set_->at(i+1)));
            ++i;

        } else {
            if(p->is<int>()) {
                p->set<int>(current_parameter_set_->at(i));
            } else {
                p->set<double>(current_parameter_set_->at(i));
            }
        }
    }
}

void OptimizerDE::reset()
{
    generation_ = 0;
    individual_ = 0;
}

void OptimizerDE::finish()
{
    AbstractOptimizer::finish();
    progress_individual_->setProgress(individual_, generation_ > 0 ? individuals_later_ : individuals_);
}
