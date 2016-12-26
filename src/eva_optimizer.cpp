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
#include <csapex/param/output_progress_parameter.h>
#include <csapex/msg/end_of_sequence_message.h>

/// SYSTEM
#include <boost/lexical_cast.hpp>

CSAPEX_REGISTER_CLASS(csapex::EvaOptimizer, csapex::Node)

using namespace csapex;
using namespace csapex::connection_types;

using namespace cslibs_jcppsocket;
using namespace serialization;


EvaOptimizer::EvaOptimizer()
{
}

void EvaOptimizer::setupParameters(Parameterizable& parameters)
{
    Optimizer::setupParameters(parameters);

    parameters.addParameter(csapex::param::ParameterFactory::declareText("server name", "localhost"));
    parameters.addParameter(csapex::param::ParameterFactory::declareText("server port", "2342"));
    parameters.addParameter(csapex::param::ParameterFactory::declareText("method", "default"));

    parameters.addParameter(csapex::param::ParameterFactory::declareRange("generations", -1, 1024, -1, 1), [this](param::Parameter* p) {
        generations_ = p->as<int>();
        if(generations_ == -1) {
            progress_generation_->setProgress(0, 0);
        } else {
            progress_generation_->setProgress(generation_, generations_);
        }
    });
    parameters.addParameter(csapex::param::ParameterFactory::declareRange("individuals/initial", 60, 60, 60, 1), individuals_initial_);
    parameters.addParameter(csapex::param::ParameterFactory::declareRange("individuals/later", 30, 30, 30, 1), individuals_later_);


    csapex::param::Parameter::Ptr pg = csapex::param::ParameterFactory::declareOutputProgress("generation");
    progress_generation_ = dynamic_cast<param::OutputProgressParameter*>(pg.get());
    parameters.addParameter(pg);
    csapex::param::Parameter::Ptr pp = csapex::param::ParameterFactory::declareOutputProgress("population");
    progress_individual_ = dynamic_cast<param::OutputProgressParameter*>(pp.get());
    parameters.addParameter(pp);
}

bool EvaOptimizer::generateNextParameterSet()
{
    // initilization?
    if(!client_) {
        tryMakeSocket();

        if(!client_) {
            aerr << "couldn't create client" << std::endl;
            node_modifier_->setError("no client");
            return false;
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
                YAML::Node description = makeRequest();
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

    if(!client_) {
        aerr << "connection lost" << std::endl;
        throw std::runtime_error("connection lost");
    }

    // send fitness back to eva
    requestNewValues(fitness_);

    return true;
}

void EvaOptimizer::updateParameters(const cslibs_jcppsocket::VectorMsg<double>::Ptr& values)
{
    if(!values) {
        return;
    }

    std::vector<csapex::param::Parameter::Ptr> supported_params;

    for(csapex::param::Parameter::Ptr p : getPersistentParameters()) {
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
        std::stringstream msg;
        msg << "number of parameters is wrong: " << values->size() << " vs. " << supported_params.size() << std::endl;
        throw std::runtime_error(msg.str());
    }

    // set parameter values to the values specified by eva
    for(std::size_t i = 0; i < supported_params.size(); ++i) {
        csapex::param::Parameter::Ptr p = supported_params[i];

        param::IntervalParameter::Ptr interval = std::dynamic_pointer_cast<param::IntervalParameter>(p);
        if(interval) {
            auto low = interval;
            auto high = std::dynamic_pointer_cast<param::IntervalParameter>(supported_params[i]);

            if(!high) {
                aerr << "cannot deserialize interval..." << std::endl;
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
    VectorMsg<char>::Ptr continue_question = std::dynamic_pointer_cast<VectorMsg<char> >(res);
    if(continue_question) {
        VectorMsg<char>::Ptr continue_answer(new VectorMsg<char>);
        if(generations_ == -1 || generation_++ < generations_) {
            continue_answer->assign("continue",8);
            client_->write(continue_answer);

            if(generations_ == -1) {
                progress_generation_->setProgress(0, 0);
            } else {
                progress_generation_->setProgress(generation_, generations_);
            }

            individual_ = 0;

            requestNewValues(fitness_);
            handleResponse();

        } else {
            continue_answer->assign("terminate",9);
            client_->write(continue_answer);

            progress_generation_->setProgress(generations_, generations_);
        }

        return;
    }


    // read a set of parameters
    VectorMsg<double>::Ptr values = std::dynamic_pointer_cast<VectorMsg<double> >(res);
    if(!values) {
        client_.reset();
        throw std::runtime_error("didn't get parameters");
    }

    current_parameter_set_  = values;

    updateParameters(current_parameter_set_);
}

void EvaOptimizer::finish()
{
    ++individual_;
    progress_individual_->setProgress(individual_, generation_ > 0 ? individuals_later_ : individuals_initial_);

    Optimizer::finish();
}

YAML::Node EvaOptimizer::makeRequest()
{
    YAML::Node req;
    // TODO: define which are possible
    req["method"] = readParameter<std::string>("method");

    for(csapex::param::Parameter::Ptr p : getPersistentParameters()) {
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

void EvaOptimizer::reset()
{
    generation_ = 0;
    individual_ = 0;
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
