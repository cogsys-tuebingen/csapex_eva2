#ifndef EVA_OPTIMIZER_ADAPTER_H
#define EVA_OPTIMIZER_ADAPTER_H

/// PROJECT
#include <csapex/view/node/default_node_adapter.h>
#include <csapex/view/utility/widget_picker.h>
#include <csapex/param/parameter.h>

/// COMPONENT
#include "eva_optimizer.h"

class QDialog;

namespace csapex {

class EvaOptimizerAdapter : public QObject, public DefaultNodeAdapter
{
    Q_OBJECT

public:
    EvaOptimizerAdapter(NodeHandleWeakPtr worker, NodeBox* parent, std::weak_ptr<EvaOptimizer> node);
    ~EvaOptimizerAdapter();

    virtual void setupUi(QBoxLayout* layout);

public Q_SLOTS:
    void pickParameter();
    void widgetPicked();

    void createParameter();

    void setNextParameterType(const QString& type);

private:
    QDialog* makeTypeDialog();

protected:
    std::weak_ptr<EvaOptimizer> wrapped_;
    DesignerScene* designer_;

    WidgetPicker widget_picker_;

    std::string next_type_;
};

}
#endif // EVA_OPTIMIZER_ADAPTER_H
