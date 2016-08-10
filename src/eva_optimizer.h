#ifndef EVA_OPTIMIZER_H
#define EVA_OPTIMIZER_H

/// PROJECT
#include <csapex/msg/msg_fwd.h>
#include <csapex/signal/signal_fwd.h>
#include <csapex/model/tickable_node.h>
#include <cslibs_jcppsocket/cpp/sync_client.h>

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

    virtual void processMarker(const csapex::connection_types::MessageConstPtr &marker) override;

private:
    void tryMakeSocket();
    void makeSocket();

private:
    void start();
    void stop();

    void finish();

    void setBest();
    void updateParameters(const cslibs_jcppsocket::VectorMsg<double>::Ptr& values);

    YAML::Node makeRequest();
    void handleResponse();
    void requestNewValues(double fitness);

private:
    Input* in_fitness_;
    Output* out_last_fitness_;
    Output* out_best_fitness_;
    Event* trigger_start_evaluation_;

    param::OutputProgressParameter* progress_individual_;
    param::OutputProgressParameter* progress_generation_;
    int generation_;
    int generations_;
    int individual_;
    int individuals_initial_;
    int individuals_later_;

    double fitness_;
    double last_fitness_;
    double best_fitness_;

    bool must_reinitialize_;
    bool do_optimization_;
    bool next_tick_;
    cslibs_jcppsocket::SyncClient::Ptr client_;


    cslibs_jcppsocket::VectorMsg<double>::Ptr current_parameter_set_;
    cslibs_jcppsocket::VectorMsg<double>::Ptr best_parameter_set_;
};


}

#endif // EVA_OPTIMIZER_H
