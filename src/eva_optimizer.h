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

    void setupParameters();
    void setup();
    void process();
    void tick();
    bool canTick();

private:
    void tryMakeSocket();
    void makeSocket();

private:
    void start();
    void stop();

    YAML::Node makeRequest();

private:
    Input* in_;
    Output* out_;

    bool can_read_;
    utils_jcppsocket::SyncClient::Ptr client_;
};


}

#endif // EVA_OPTIMIZER_H
