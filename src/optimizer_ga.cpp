#include "optimizer_ga.h"

using namespace csapex;

#include "optimizer_de.h"

#include <csapex/param/range_parameter.h>
#include <csapex/param/interval_parameter.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/param/output_progress_parameter.h>
#include <csapex/param/value_parameter.h>

using namespace csapex;
using namespace cslibs_jcppsocket;

OptimizerGA::OptimizerGA()
{

}

std::string OptimizerGA::getName() const
{
    return "GA";
}


void OptimizerGA::getOptions(YAML::Node &options)
{
    AbstractOptimizer::getOptions(options);

    options["individuals/later_generations"] = individuals_later_;
}


void OptimizerGA::addParameters(Parameterizable& params)
{
    AbstractOptimizer::addParameters(params);

    params.addTemporaryParameter(param::ParameterFactory::declareRange("individuals/later_generations", 30, 30, 30, 1),
                                 individuals_later_);

    params.addTemporaryParameter(csapex::param::ParameterFactory::declareRange("generations", -1, 1024, -1, 1), [this](param::Parameter* p) {
        generations_ = p->as<int>();
        if(generations_ == -1) {
            progress_generation_->setProgress(0, 0);
        } else {
            progress_generation_->setProgress(generation_, generations_);
        }
    });

    param::Parameter::Ptr pg = csapex::param::ParameterFactory::declareOutputProgress("generation");
    progress_generation_ = dynamic_cast<param::OutputProgressParameter*>(pg.get());
    params.addTemporaryParameter(pg);
}

bool OptimizerGA::canContinue() const
{
    return generations_ == -1 || generation_ < generations_;
}

void OptimizerGA::nextIteration()
{
    if(generations_ == -1) {
        progress_generation_->setProgress(0, 0);
    } else {
        ++generation_;
        progress_generation_->setProgress(generation_, generations_);
    }

    AbstractOptimizer::nextIteration();
}

void OptimizerGA::terminate()
{
    progress_generation_->setProgress(generations_, generations_);
}

namespace {
std::size_t getNrOfBits(const csapex::param::Parameter* p)
{

    if(auto range = dynamic_cast<const param::RangeParameter*>(p)){
        if(range->is<double>()){
            double min = range->min<double>();
            double max = range->max<double>();
            double step = range->step<double>();

            std::size_t steps = std::ceil((max-min)/step);
            return std::ceil(std::log2(steps));
        }
        else if(range->is<int>()){
            int min = range->min<int>();
            int max = range->max<int>();
            int step = range->step<int>();

            std::size_t steps = std::ceil((max-min)/(double)step);
            return std::ceil(std::log2(steps));
        }

    }
    if(auto valname = dynamic_cast<const param::ValueParameter*>(p)){
        if(valname->is<double>()){
            return 64;
        }
        else if(valname->is<int>()){
            return 32;
        }
        else if(valname->is<bool>()){
            return 1;
        }

    }
    return 0;
}

int readParameterValue(csapex::param::Parameter* p, const char* buffer, int first_bit)
{
    int len = getNrOfBits(p);

    if(auto range = dynamic_cast<param::RangeParameter*>(p)){
        long result = 0;
        for(int b = 0; b < len; ++b) {
            std::size_t byte = (first_bit + b) / 8;
            std::size_t bit = (first_bit + b) % 8;

            bool entry = buffer[byte] & (1 << bit);
            if(entry) {
                result += std::pow(2, b);
            }
        }

        if(range->is<double>()){
            double min = range->min<double>();
            double max = range->max<double>();
            double step = range->step<double>();

            std::size_t steps = std::ceil((max-min)/step);

            double value = min + ((result % steps) * step);

            apex_assert(value >= min);
            apex_assert(value <= max);

            p->set<double>(value);
        }
        else if(range->is<int>()){
            int min = range->min<int>();
            int max = range->max<int>();
            int step = range->step<int>();

            std::size_t steps = std::ceil((max-min)/step);

            int value = min + ((result % steps) * step);

            apex_assert(value >= min);
            apex_assert(value <= max);

            p->set<int>(value);
        }
        else {
            throw std::runtime_error(std::string("unsupported parameter type ") + p->type2string(p->type()) );
        }
    }
    else if(auto value = dynamic_cast<param::ValueParameter*>(p)){
        if(p->is<bool>()) {
            std::size_t byte = (first_bit) / 8;
            std::size_t bit = (first_bit) % 8;

            bool entry = buffer[byte] & (1 << bit);
            p->set<bool>(entry);
        }
        else if(p->is<int>()) {
            int result = 0;
            for(int b = 0; b < len; ++b) {
                std::size_t byte = (first_bit + b) / 8;
                std::size_t bit = (first_bit + b) % 8;

                bool entry = buffer[byte] & (1 << bit);
                if(entry) {
                    result += std::pow(2, b);
                }
            }
            std::cerr << "read int " << result << std::endl;;
            p->set<int>(result);
        }
        else {
            throw std::runtime_error(std::string("unsupported parameter type ") + p->type2string(p->type()) );
        }
    }
    else {
        throw std::runtime_error(std::string("unsupported parameter type ") + p->type2string(p->type()) );
    }
    return len;
}
}

void OptimizerGA::encodeParameters(const std::vector<param::ParameterPtr>& params, YAML::Node &out)
{
    std::size_t n_bits = 0;
    for(csapex::param::Parameter::Ptr p : params) {
        n_bits += getNrOfBits(p.get());
    }

    out["problem_dimension"] = n_bits;
}

void OptimizerGA::decodeParameters(const cslibs_jcppsocket::SocketMsg::Ptr &msg,
                                   const std::vector<param::ParameterPtr>& params)
{
    std::size_t needed_bits = 0;
    for(csapex::param::Parameter::Ptr p : params) {
        needed_bits += getNrOfBits(p.get());
    }

    std::size_t available_bits = msg->byteSize() * 8;
    apex_assert_msg(available_bits >= needed_bits,
                    (std::string("not enough bits: ") + std::to_string(available_bits) +
                    " available, " + std::to_string(needed_bits) + " needed").c_str());

    VectorMsg<char>::Ptr string_message = std::dynamic_pointer_cast<VectorMsg<char> >(msg);
    apex_assert(string_message);

    const char* buffer = &*string_message->begin();

    std::size_t first_bit = 0;
    for(csapex::param::Parameter::Ptr p : params) {
        first_bit += readParameterValue(p.get(), buffer, first_bit);
    }
//    std::size_t n_bits = (n_bits / 8 + 1) * 8;
//    for(csapex::param::Parameter::Ptr p : params) {

//        std::size_t bits_needed = getNrOfBits(p.get());
//        if( param::RangeParameter::Ptr range = std::dynamic_pointer_cast<param::RangeParameter>(p)){
//            if(range->is<double>()){
//                double min = range->min<double>();
//                double max = range->max<double>();
//                double step = range->step<double>();

//                return std::ceil((max-min)/step);
//            }
//            else if(range->is<int>()){
//                int min = range->min<int>();
//                int max = range->max<int>();
//                int step = range->step<int>();

//                return std::ceil((max-min)/(double)step);
//            }
//        }

//    }



}

void OptimizerGA::reset()
{
    generation_ = 0;
    individual_ = 0;
}

void OptimizerGA::finish()
{
    AbstractOptimizer::finish();
    progress_individual_->setProgress(individual_, generation_ > 0 ? individuals_later_ : individuals_);
}
