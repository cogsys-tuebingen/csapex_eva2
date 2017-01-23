#include "abstract_optimizer.h"
#include <csapex/param/parameter_factory.h>
#include <csapex/param/output_progress_parameter.h>

using namespace csapex;

AbstractOptimizer::AbstractOptimizer()
    : individual_(0)
{

}

AbstractOptimizer::~AbstractOptimizer()
{

}

void AbstractOptimizer::getOptions(YAML::Node &options)
{
    options["individuals"] = individuals_;
}

void AbstractOptimizer::addParameters(Parameterizable &params)
{
    params.addTemporaryParameter(param::ParameterFactory::declareRange("individuals/individuals", 60, 60, 60, 1),
                                 individuals_);

    param::Parameter::Ptr pp = csapex::param::ParameterFactory::declareOutputProgress("population");
    progress_individual_ = dynamic_cast<param::OutputProgressParameter*>(pp.get());
    params.addTemporaryParameter(pp);
}

void AbstractOptimizer::nextIteration()
{
    individual_ = 0;
}

void AbstractOptimizer::terminate()
{

}

void AbstractOptimizer::finish()
{
    ++individual_;
    progress_individual_->setProgress(individual_, individuals_);
}
