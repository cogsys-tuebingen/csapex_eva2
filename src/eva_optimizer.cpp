/// HEADER
#include "eva_optimizer.h"

/// PROJECT
#include <csapex/msg/io.h>
#include <csapex/signal/trigger.h>
#include <csapex/utility/register_apex_plugin.h>
#include <utils_param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <utils_param/range_parameter.h>
#include <utils_jcppsocket/cpp/socket_msgs.h>

/// SYSTEM
#include <boost/lexical_cast.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaOptimizer, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;

using namespace utils_jcppsocket;
using namespace serialization;


EvaOptimizer::EvaOptimizer()
    : can_read_(true), do_optimization_(false)
{
}

void EvaOptimizer::setupParameters(Parameterizable& parameters)
{
    parameters.addParameter(param::ParameterFactory::declareText("server name", "localhost"));
    parameters.addParameter(param::ParameterFactory::declareText("server port", "2342"));
    parameters.addParameter(param::ParameterFactory::declareText("method", "default"));
}

void EvaOptimizer::setup(NodeModifier& node_modifier)
{
    in_  = node_modifier.addInput<double>("Fitness");
    out_ = node_modifier.addTrigger("Evaluate");


    modifier_->setIsSource(true);
    modifier_->setIsSink(true);
}

bool EvaOptimizer::canTick()
{
    return can_read_ && do_optimization_;
}

void EvaOptimizer::tick()
{
    assert(can_read_);

    // initilization?
    if(!client_) {
        tryMakeSocket();

        if(!client_) {
            modifier_->setError("no client");
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

    SocketMsg::Ptr res;
    client_->read(res);

    if(!res) {
        client_.reset();
        throw std::runtime_error("could not read");
    }

    ErrorMsg::Ptr err = std::dynamic_pointer_cast<ErrorMsg>(res);
    if(err) {
        client_.reset();

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
        throw std::runtime_error("didn't get parameters");
    }

    std::vector<param::Parameter::Ptr> supported_params;
    foreach(param::Parameter::Ptr p, getParameters()) {
        // TODO: support more than double ranges
        param::RangeParameter::Ptr dbl_range = std::dynamic_pointer_cast<param::RangeParameter>(p);
        if(!dbl_range || !dbl_range->is<double>()) {
            continue;
        }
        supported_params.push_back(p);
    }

    if(values->size() != supported_params.size()) {
        client_.reset();
        std::stringstream msg;
        msg << "number of parameters is wrong: " << values->size() << " vs. " << supported_params.size() << std::endl;
        throw std::runtime_error(msg.str());
    }

//    ainfo << "got a message with " << values->size() << " doubles" << std::endl;

    // set parameter values to the values specified by eva
    for(std::size_t i = 0; i < supported_params.size(); ++i) {
        param::Parameter::Ptr p = supported_params[i];
        p->set<double>(values->at(i));
    }

    can_read_ = false;

    // start another evaluation
    out_->trigger();
}

void EvaOptimizer::process()
{
    if(can_read_) {
        return;
    }

    double fitness = msg::getValue<double>(in_);
    ainfo << "got fitness: " << fitness << std::endl;

    // send fitness back to eva
    ValueMsg<double>::Ptr msg(new ValueMsg<double>);
    msg->set(fitness);
    client_->write(msg);

    can_read_ = true;
}

YAML::Node EvaOptimizer::makeRequest()
{
    YAML::Node req;
    // TODO: make parameter
    // TODO: define which are possible
    req["method"] = readParameter<std::string>("method");

    foreach(param::Parameter::Ptr p, getParameters()) {
        // TODO: support more than double ranges
        param::RangeParameter::Ptr dbl_range = std::dynamic_pointer_cast<param::RangeParameter>(p);
        if(!dbl_range || !dbl_range->is<double>()) {
            continue;
        }

        YAML::Node param_node;
        param_node["name"] = p->name();
        param_node["type"] = "double/range";
        param_node["min"] = dbl_range->min<double>();
        param_node["max"] = dbl_range->max<double>();
        param_node["step"] = dbl_range->step<double>();

        req["params"].push_back(param_node);
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

    modifier_->setNoError();
}
