#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// PROJECT
#include <csapex/model/tickable_node.h>
#include <utils_jcppsocket/cpp/sync_client.h>

namespace csapex {


class EvaOptimizer : public csapex::TickableNode
{
    friend class EvaOptimizerAdapter;

public:
    EvaOptimizer();

    virtual void setupParameters(Parameterizable& parameters) override;
    virtual void setup(csapex::NodeModifier& node_modifier) override;
    virtual void process() override;
    virtual void tick() override;
    virtual bool canTick() override;

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
