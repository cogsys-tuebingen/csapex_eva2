#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// PROJECT
#include <csapex_optimization/optimizer.h>
#include <csapex/msg/msg_fwd.h>
#include <csapex/signal/signal_fwd.h>
#include <cslibs_jcppsocket/cpp/sync_client.h>
#include "abstract_optimizer.h"

namespace csapex {


class EvaOptimizer : public Optimizer
{
    friend class EvaOptimizerAdapter;

    enum class Method
    {
        None,
        DE,
        GA
    };

public:
    EvaOptimizer();

    virtual void setupParameters(Parameterizable& parameters) override;
    virtual bool generateNextParameterSet() override;

private:
    void tryMakeSocket();
    void makeSocket();

private:
    void reset();
    void start() override;

    void finish();


    void handleResponse();
    void requestNewValues(double fitness);

    void updateOptimizer();

private:
    Method method_;

    std::shared_ptr<AbstractOptimizer> optimizer_;

    cslibs_jcppsocket::SyncClient::Ptr client_;
};


}

#endif // EVA_OPTIMIZER_H
