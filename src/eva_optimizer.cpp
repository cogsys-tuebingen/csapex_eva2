/// HEADER
#include "eva_optimizer.h"

/// PROJECT
#include <csapex/msg/io.h>
#include <csapex/signal/trigger.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <csapex/param/range_parameter.h>
#include <csapex/param/interval_parameter.h>
#include <utils_jcppsocket/cpp/socket_msgs.h>

/// SYSTEM
#include <boost/lexical_cast.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaOptimizer, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;

using namespace utils_jcppsocket;
using namespace serialization;


EvaOptimizer::EvaOptimizer()
    : last_fitness_(std::numeric_limits<double>::infinity()),
      best_fitness_(std::numeric_limits<double>::infinity()),
      must_reinitialize_(true), do_optimization_(false)
{
}

void EvaOptimizer::setupParameters(Parameterizable& parameters)
{
    parameters.addParameter(csapex::param::ParameterFactory::declareText("server name", "localhost"));
    parameters.addParameter(csapex::param::ParameterFactory::declareText("server port", "2342"));
    parameters.addParameter(csapex::param::ParameterFactory::declareText("method", "default"));

    parameters.addParameter(csapex::param::ParameterFactory::declareTrigger("start"), [this](csapex::param::Parameter*) {
        start();
    });
    parameters.addParameter(csapex::param::ParameterFactory::declareTrigger("finish"), [this](csapex::param::Parameter*) {
        finish();
    });
    parameters.addParameter(csapex::param::ParameterFactory::declareTrigger("stop"), [this](csapex::param::Parameter*) {
        stop();
    });
}

void EvaOptimizer::setup(NodeModifier& node_modifier)
{
    in_fitness_  = node_modifier.addInput<double>("Fitness");
    out_last_fitness_  = node_modifier.addOutput<double>("Last Fitness");
    out_best_fitness_  = node_modifier.addOutput<double>("Best Fitness");
    trigger_start_evaluation_ = node_modifier.addTrigger("Evaluate");


    node_modifier.setIsSource(true);
    node_modifier.setIsSink(true);
}

bool EvaOptimizer::canTick()
{
    return must_reinitialize_ && do_optimization_;
}

void EvaOptimizer::tick()
{
   // assert(must_reinitialize_);

    // initilization?
    if(!client_) {
        tryMakeSocket();

        if(!client_) {
            node_modifier_->setError("no client");
            return;
        }

        if(!client_->isConnected()) {
            if(!client_->connect()) {
                throw std::runtime_error("Couldn't connect!");
                client_.reset();
            }
        }

        // connect to eva
        SocketMsg::Ptr res;
        if(client_->read(res)) {
            ErrorMsg::Ptr err = std::dynamic_pointer_cast<ErrorMsg>(res);
            VectorMsg<char>::Ptr welcome = std::dynamic_pointer_cast<VectorMsg<char> >(res);

            if(err) {
                client_.reset();

                std::stringstream ss;
                ss << "Got error [ " << err->get() << " ]";
                throw std::runtime_error(ss.str());
            }

            if(welcome) {
                //                ainfo << "connection established: " << std::string(welcome->begin(), welcome->end()) << std::endl;

                // generate request
                YAML::Node description = makeRequest();
                //                ainfo << "optimization request:\n" << description << std::endl;

                // send parameter description
                SocketMsg::Ptr res;
                VectorMsg<char>::Ptr config(new VectorMsg<char>);
                std::stringstream config_ss;
                config_ss << description;

                std::string yaml = config_ss.str();
                config->assign(yaml.data(), yaml.size());

                client_->write(config);

            } else {
                client_.reset();
            }
        } else {
            client_.reset();
            throw std::runtime_error("socket error");
        }
    }

    if(!client_) {
        throw std::runtime_error("connection lost");
    }

    must_reinitialize_ = false;

    handleEvaResponse();
}

void EvaOptimizer::handleEvaResponse()
{
    fitness_ = std::numeric_limits<double>::infinity();

    SocketMsg::Ptr res;
    client_->read(res);

    if(!res) {
        client_.reset();
        must_reinitialize_ = true;

        throw std::runtime_error("could not read");
    }

    ErrorMsg::Ptr err = std::dynamic_pointer_cast<ErrorMsg>(res);
    if(err) {
        client_.reset();
        must_reinitialize_ = true;

        std::stringstream ss;
        ss << "Got error [ " << err->get() << " ]";
        throw std::runtime_error(ss.str());
    }

    // continue message?
    VectorMsg<char>::Ptr continue_question = std::dynamic_pointer_cast<VectorMsg<char> >(res);
    if(continue_question) {
        //        ainfo << "continue?" << std::endl;
        VectorMsg<char>::Ptr continue_answer(new VectorMsg<char>);
        continue_answer->assign("continue",8);
        client_->write(continue_answer);
        tick();
        return;
    }


    // read a set of parameters
    VectorMsg<double>::Ptr values = std::dynamic_pointer_cast<VectorMsg<double> >(res);
    if(!values) {
        client_.reset();
        must_reinitialize_ = true;
        throw std::runtime_error("didn't get parameters");
    }

    std::vector<csapex::param::Parameter::Ptr> supported_params;
    for(csapex::param::Parameter::Ptr p : getParameters()) {
        // TODO: support more types
        param::RangeParameter::Ptr range = std::dynamic_pointer_cast<param::RangeParameter>(p);
        if(range) {
            if(range->is<double>()) {
                supported_params.push_back(p);

            } else if(range->is<int>()) {
                supported_params.push_back(p);
            }
        }

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval && interval->is<std::pair<int, int>>()) {
            supported_params.push_back(p);
            supported_params.push_back(p);
        }
    }

    if(values->size() != supported_params.size()) {
        client_.reset();
        must_reinitialize_ = true;
        std::stringstream msg;
        msg << "number of parameters is wrong: " << values->size() << " vs. " << supported_params.size() << std::endl;
        throw std::runtime_error(msg.str());
    }

    //    ainfo << "got a message with " << values->size() << " doubles" << std::endl;

    // set parameter values to the values specified by eva
    for(std::size_t i = 0; i < supported_params.size(); ++i) {
        csapex::param::Parameter::Ptr p = supported_params[i];

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval) {
            auto low = interval;
            auto high = std::dynamic_pointer_cast<param::IntervalParameter>(supported_params[i]);

            if(!high) {
                aerr << "cannot deserialize interaval..." << std::endl;
            } else {
                p->set<std::pair<int, int>>(std::pair<int, int>(values->at(i), values->at(i+1)));
            }
            ++i;

        } else {
            if(p->is<int>()) {
                p->set<int>(values->at(i));
            } else {
                p->set<double>(values->at(i));
            }
        }
    }

    // start another evaluation
    trigger_start_evaluation_->trigger();
}

void EvaOptimizer::process()
{
    if(must_reinitialize_) {
        return;
    }
    fitness_ = msg::getValue<double>(in_fitness_);

    if(best_fitness_ != std::numeric_limits<double>::infinity()) {
        msg::publish(out_best_fitness_, best_fitness_);
    }
    if(last_fitness_ != std::numeric_limits<double>::infinity()) {
        msg::publish(out_last_fitness_, last_fitness_);
    }
}

void EvaOptimizer::finish()
{
    if(!client_) {
        return;
    }

    ainfo << "got fitness: " << fitness_ << std::endl;

    last_fitness_ = fitness_;

    if(fitness_ < best_fitness_) {
        best_fitness_ = fitness_;
        // TODO: save parameters
    }


    // send fitness back to eva
    ValueMsg<double>::Ptr msg(new ValueMsg<double>);
    msg->set(fitness_);
    client_->write(msg);

    handleEvaResponse();
}

YAML::Node EvaOptimizer::makeRequest()
{
    YAML::Node req;
    // TODO: make parameter
    // TODO: define which are possible
    req["method"] = readParameter<std::string>("method");

    for(csapex::param::Parameter::Ptr p : getParameters()) {
        param::RangeParameter::Ptr range = std::dynamic_pointer_cast<param::RangeParameter>(p);
        if(range) {
            if(range->is<double>()) {
                YAML::Node param_node;
                param_node["name"] = p->name();
                param_node["type"] = "double/range";
                param_node["min"] = range->min<double>();
                param_node["max"] = range->max<double>();
                param_node["step"] = range->step<double>();

                req["params"].push_back(param_node);
                continue;

            } else if(range->is<int>()) {
                YAML::Node param_node;
                param_node["name"] = p->name();
                param_node["type"] = "double/range";
                param_node["min"] = (double) range->min<int>();
                param_node["max"] = (double)range->max<int>();
                param_node["step"] = (double)range->step<int>();

                req["params"].push_back(param_node);
                continue;
            }
        }

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval && interval->is<std::pair<int, int>>()) {
            YAML::Node node_low;
            node_low["name"] = p->name();
            node_low["type"] = "double/range";
            node_low["min"] = (double) interval->min<int>();
            node_low["max"] = (double) interval->max<int>();
            node_low["step"] = (double) interval->step<int>();

            YAML::Node node_high;
            node_high["name"] = p->name();
            node_high["type"] = "double/range";
            node_high["min"] = (double) interval->min<int>();
            node_high["max"] = (double) interval->max<int>();
            node_high["step"] = (double) interval->step<int>();

            req["params"].push_back(node_low);
            req["params"].push_back(node_high);
            continue;
        }
    }
    return req;
}

void EvaOptimizer::start()
{
    ainfo << "starting optimization" << std::endl;
    do_optimization_ = true;
}


void EvaOptimizer::stop()
{
    ainfo << "stopping optimization" << std::endl;
    do_optimization_ = false;
    must_reinitialize_ = true;
}


void EvaOptimizer::tryMakeSocket()
{
    std::string str_name = readParameter<std::string>("server name");
    std::string str_port = readParameter<std::string>("server port");

    if(!str_name.empty() && !str_port.empty()) {
        makeSocket();
    }
}

void EvaOptimizer::makeSocket()
{
    std::string str_name = readParameter<std::string>("server name");
    std::string str_port = readParameter<std::string>("server port");
    int         port = boost::lexical_cast<int>(str_port);

    client_.reset(new utils_jcppsocket::SyncClient(str_name, port));

    node_modifier_->setNoError();
}
