#ifndef EVA_OPTIMIZER_ADAPTER_H
#define EVA_OPTIMIZER_ADAPTER_H

/// PROJECT
#include <csapex/view/default_node_adapter.h>

/// COMPONENT
#include "eva_optimizer.h"

class QDialog;

namespace csapex {

class EvaOptimizerAdapter : public QObject, public DefaultNodeAdapter
{
    Q_OBJECT

public:
    EvaOptimizerAdapter(NodeWorker *worker, EvaOptimizer *node, WidgetController *widget_ctrl);

    virtual void setupUi(QBoxLayout* layout);

public Q_SLOTS:
    void createParameter();

    void setNextParameterType(const QString& type);

private:
    QDialog* makeTypeDialog();

protected:
    EvaOptimizer* wrapped_;

    std::string next_type_;
};

}
#endif // EVA_OPTIMIZER_ADAPTER_H
