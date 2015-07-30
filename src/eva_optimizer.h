#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// PROJECT
#include <csapex/model/node.h>
#include <utils_jcppsocket/cpp/sync_client.h>

namespace csapex {


class EvaOptimizer : public csapex::Node
{
    friend class EvaOptimizerAdapter;

public:
    EvaOptimizer();

    void setupParameters(Parameterizable& parameters);
    void setup(csapex::NodeModifier& node_modifier) override;
    virtual void process() override;
    void tick();
    bool canTick();

private:
    void tryMakeSocket();
    void makeSocket();

private:
    void start();
    void stop();

    void finish();

    YAML::Node makeRequest();
    void handleEvaResponse();

private:
    Input* in_fitness_;
    Trigger* trigger_start_evaluation_;

    double fitness_;
    bool must_reinitialize_;
    bool do_optimization_;
    utils_jcppsocket::SyncClient::Ptr client_;
};


}

#endif // EVA_OPTIMIZER_H
