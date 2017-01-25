/// HEADER
#include "eva_optimizer.h"

/// PROJECT
#include <csapex/msg/io.h>
#include <csapex/signal/event.h>
#include <csapex/utility/register_apex_plugin.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/model/node_modifier.h>
#include <csapex/param/range_parameter.h>
#include <csapex/param/interval_parameter.h>
#include <cslibs_jcppsocket/cpp/socket_msgs.h>
#include <csapex/msg/end_of_sequence_message.h>
#include "optimizer_de.h"
#include "optimizer_ga.h"

/// SYSTEM
#include <boost/lexical_cast.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaOptimizer, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;

using namespace cslibs_jcppsocket;
using namespace serialization;


EvaOptimizer::EvaOptimizer()
    : method_(Method::None)
{
}

void EvaOptimizer::setupParameters(Parameterizable& parameters)
{
    Optimizer::setupParameters(parameters);

    parameters.addParameter(param::ParameterFactory::declareText("server name", "localhost"));
    parameters.addParameter(param::ParameterFactory::declareText("server port", "2342"));

    std::map<std::string, int> methods {
        {"Differential Evolution", (int) Method::DE},
        {"Genetic Algorithm", (int) Method::GA}
    };
    parameters.addParameter(param::ParameterFactory::declareParameterSet("method", methods, (int) Method::DE),
                            [this](param::Parameter* p){
        updateOptimizer();
    });
}

bool EvaOptimizer::generateNextParameterSet()
{
    apex_assert(optimizer_);

    if(!client_) {
        aerr << "connection lost" << std::endl;
        throw std::runtime_error("connection lost");
    }

    // send fitness back to eva
    requestNewValues(fitness_);

    return true;
}



void EvaOptimizer::requestNewValues(double fitness)
{
    ValueMsg<double>::Ptr msg(new ValueMsg<double>);
    msg->set(fitness);

    client_->write(msg);
    handleResponse();
}

void EvaOptimizer::handleResponse()
{
    SocketMsg::Ptr res;
    client_->read(res);

    if(!res) {
        client_.reset();

        throw std::runtime_error("could not read");
    }

    ValueMsg<double>::Ptr value = std::dynamic_pointer_cast<ValueMsg<double>>(res);
    if(value) {
        ainfo << "finished with fitness " << value->get() << std::endl;
        stop();

        setBest();
        return;
    }


    ErrorMsg::Ptr err = std::dynamic_pointer_cast<ErrorMsg>(res);
    if(err) {
        client_.reset();

        std::stringstream ss;
        ss << "Got error [ " << err->get() << " ]";
        throw std::runtime_error(ss.str());
    }

    // continue message?
    VectorMsg<char>::Ptr string_message = std::dynamic_pointer_cast<VectorMsg<char> >(res);
    if(string_message) {
        std::string question(string_message->begin(), string_message->end());
        if(question == "continue") {
            VectorMsg<char>::Ptr continue_answer(new VectorMsg<char>);
            if(optimizer_->canContinue()) {
                continue_answer->assign("continue",8);
                client_->write(continue_answer);

                optimizer_->nextIteration();

                requestNewValues(fitness_);
                handleResponse();

            } else {
                continue_answer->assign("terminate",9);
                client_->write(continue_answer);

                optimizer_->terminate();
            }
            return;
        }
    }

    apex_assert(optimizer_);

    try {
        optimizer_->decodeParameters(res, getPersistentParameters());
    } catch(...) {
        client_.reset();
        throw;
    }
}

void EvaOptimizer::finish()
{
    if(optimizer_) {
        optimizer_->finish();
    }

    Optimizer::finish();
}


void EvaOptimizer::reset()
{
    if(optimizer_) {
        optimizer_->reset();
    }
}

void EvaOptimizer::start()
{
    // initilization?
    if(!client_) {
        tryMakeSocket();

        if(!client_) {
            throw std::runtime_error("couldn't create client");
        } else {
            ainfo << "client initialized" << std::endl;
        }

        if(!client_->isConnected()) {
            if(!client_->connect()) {
                aerr << "could not connect to EvA2" << std::endl;
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
                YAML::Node description;
                description["method"] = optimizer_->getName();

                YAML::Node options;
                optimizer_->getOptions(options);
                description["options"] = options;

                optimizer_->encodeParameters(getPersistentParameters(), description);
                //                ainfo << "optimization request:\n" << description << std::endl;

                // send parameter description
                SocketMsg::Ptr res;
                VectorMsg<char>::Ptr config(new VectorMsg<char>);
                std::stringstream config_ss;
                config_ss << description;

                std::string yaml = config_ss.str();
                config->assign(yaml.data(), yaml.size());

                ainfo << "write config " << std::endl;
                client_->write(config);

                handleResponse();

            } else {
                aerr << "didn't receive a welcome message" << std::endl;
                client_.reset();
            }
        } else {
            aerr << "socket error" << std::endl;
            client_.reset();
            throw std::runtime_error("socket error");
        }
    }

    Optimizer::start();
}

void EvaOptimizer::updateOptimizer()
{
    Method method = static_cast<Method>(readParameter<int>("method"));

    if(method_ != method) {
        method_ = method;

        if(optimizer_) {
            stop();
            optimizer_.reset();
        }

        removeTemporaryParameters();

        switch(method_) {
        default:
        case Method::DE:
            optimizer_ = std::make_shared<OptimizerDE>();
            break;
        case Method::GA:
            optimizer_ = std::make_shared<OptimizerGA>();
            break;
        }

        optimizer_->addParameters(*this);
    }
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

    client_.reset(new cslibs_jcppsocket::SyncClient(str_name, port));

    node_modifier_->setNoError();
}
