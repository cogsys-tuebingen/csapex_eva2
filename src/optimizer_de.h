#ifndef OPTIMIZER_DE_H
#define OPTIMIZER_DE_H

#include "abstract_optimizer.h"

namespace csapex
{

class OptimizerDE : public AbstractOptimizer
{
public:
    OptimizerDE();

    std::string getName() const override;

    bool canContinue() const override;
    void nextIteration() override;
    void terminate() override;

    void getOptions(YAML::Node &options) override;
    void addParameters(Parameterizable& params) override;

    void encodeParameters(const std::vector<param::ParameterPtr>& params,
                          YAML::Node& out) override;

    void decodeParameters(const cslibs_jcppsocket::SocketMsg::Ptr& msg,
                          const std::vector<param::ParameterPtr> &params) override;

    void reset() override;
    void finish(double fitness, double best_fitness, double worst_fitness) override;

private:
    cslibs_jcppsocket::VectorMsg<double>::Ptr current_parameter_set_;

    int individuals_later_;

    param::OutputProgressParameter* progress_generation_;
    int generation_;
    int generations_;
};

}

#endif // OPTIMIZER_DE_H
