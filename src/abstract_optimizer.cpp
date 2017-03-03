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

    param::Parameter::Ptr progress_fitness = csapex::param::ParameterFactory::declareOutputProgress("fitness level");
    progress_fitness_ = dynamic_cast<param::OutputProgressParameter*>(progress_fitness.get());
    params.addTemporaryParameter(progress_fitness);

    param::Parameter::Ptr progress_population = csapex::param::ParameterFactory::declareOutputProgress("population");
    progress_individual_ = dynamic_cast<param::OutputProgressParameter*>(progress_population.get());
    params.addTemporaryParameter(progress_population);

    progress_fitness_->setProgress(0,0);
}

void AbstractOptimizer::nextIteration()
{
    individual_ = 0;
}

void AbstractOptimizer::terminate()
{

}

void AbstractOptimizer::reset()
{
    progress_fitness_->setProgress(0,100);
}

void AbstractOptimizer::finish(double fitness, double best_fitness, double worst_fitness)
{
    ++individual_;
    progress_individual_->setProgress(individual_, individuals_);

    if(worst_fitness == best_fitness) {
        progress_fitness_->setProgress(0, 100);

    } else {
        double range = worst_fitness - best_fitness;
        progress_fitness_->setProgress(100 - ((fitness - best_fitness) / range) * 100, 100);
    }
}
