#ifndef ABSTRACT_OPTIMIZER_H
#define ABSTRACT_OPTIMIZER_H

#include <csapex/param/param_fwd.h>
#include <csapex/model/parameterizable.h>
#include <yaml-cpp/yaml.h>
#include <cslibs_jcppsocket/cpp/socket_msgs.h>

namespace csapex
{

class AbstractOptimizer
{
public:
    AbstractOptimizer();
    virtual ~AbstractOptimizer();

    virtual std::string getName() const = 0;

    virtual void getOptions(YAML::Node& options);

    virtual void addParameters(Parameterizable& params);

    virtual bool canContinue() const = 0;
    virtual void nextIteration();

    virtual void terminate();

    virtual void encodeParameters(const std::vector<param::ParameterPtr>& params,
                                  YAML::Node& out) = 0;

    virtual void decodeParameters(const cslibs_jcppsocket::SocketMsg::Ptr& msg,
                                  const std::vector<param::ParameterPtr> &params) = 0;


    virtual void reset();
    virtual void finish(double fitness, double best_fitness, double worst_fitness);

protected:
    param::OutputProgressParameter* progress_fitness_;

    param::OutputProgressParameter* progress_individual_;
    int individual_;
    int individuals_;
};

}

#endif // ABSTRACT_OPTIMIZER_H
