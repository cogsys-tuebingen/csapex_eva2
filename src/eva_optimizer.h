#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// PROJECT
#include <csapex_optimization/optimizer.h>
#include <csapex/msg/msg_fwd.h>
#include <csapex/signal/signal_fwd.h>
#include <cslibs_jcppsocket/cpp/sync_client.h>

namespace csapex {


class EvaOptimizer : public Optimizer
{
    friend class EvaOptimizerAdapter;

public:
    EvaOptimizer();

    virtual void setupParameters(Parameterizable& parameters) override;
    virtual bool generateNextParameterSet() override;

private:
    void tryMakeSocket();
    void makeSocket();

private:
    void reset();

    void finish();

    void updateParameters(const cslibs_jcppsocket::VectorMsg<double>::Ptr& values);

    YAML::Node makeRequest();
    void handleResponse();
    void requestNewValues(double fitness);

private:
    param::OutputProgressParameter* progress_individual_;
    param::OutputProgressParameter* progress_generation_;
    int generation_;
    int generations_;
    int individual_;
    int individuals_initial_;
    int individuals_later_;

    cslibs_jcppsocket::SyncClient::Ptr client_;

    cslibs_jcppsocket::VectorMsg<double>::Ptr current_parameter_set_;
};


}

#endif // EVA_OPTIMIZER_H
