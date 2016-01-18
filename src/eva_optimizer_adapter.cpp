/// HEADER
#include "eva_optimizer_adapter.h"

/// COMPONENT
#include <csapex_optimization/parameter_dialog.h>

/// PROJECT
#include <csapex/view/utility/register_node_adapter.h>
#include <csapex/param/range_parameter.h>
#include <csapex/param/parameter_factory.h>
#include <csapex/command/dispatcher.h>
#include <csapex/command/add_msg_connection.h>
#include <csapex/utility/uuid_provider.h>
#include <csapex/view/designer/widget_controller.h>

/// SYSTEM
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>

using namespace csapex;

CSAPEX_REGISTER_NODE_ADAPTER(EvaOptimizerAdapter, csapex::EvaOptimizer)


EvaOptimizerAdapter::EvaOptimizerAdapter(NodeHandleWeakPtr worker, NodeBox* parent, std::weak_ptr<EvaOptimizer> node)
    : DefaultNodeAdapter(worker, parent), wrapped_(node), designer_(widget_ctrl_->getDesignerScene())
{
    QObject::connect(&widget_picker_, SIGNAL(widgetPicked()), this, SLOT(widgetPicked()));
}

EvaOptimizerAdapter::~EvaOptimizerAdapter()
{
}



void EvaOptimizerAdapter::setupUi(QBoxLayout* layout)
{
    DefaultNodeAdapter::setupUi(layout);

    QPushButton* btn_add_param = new QPushButton("Create Parameter");
    layout->addWidget(btn_add_param);
    QObject::connect(btn_add_param, SIGNAL(clicked()), this, SLOT(createParameter()));

    QPushButton* btn_pick_param = new QPushButton("Pick Parameter");
    layout->addWidget(btn_pick_param);
    QObject::connect(btn_pick_param, SIGNAL(clicked()), this, SLOT(pickParameter()));
}

QDialog* EvaOptimizerAdapter::makeTypeDialog()
{
    QVBoxLayout* layout = new QVBoxLayout;

    QFormLayout* form = new QFormLayout;

    QComboBox* type = new QComboBox;
    type->addItem("double");
    form->addRow("Type", type);

    QObject::connect(type, SIGNAL(currentIndexChanged(QString)), this, SLOT(setNextParameterType(QString)));
    next_type_ = type->currentText().toStdString();

    layout->addLayout(form);

    QDialogButtonBox* buttons = new QDialogButtonBox;
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);


    QDialog* dialog = new QDialog;
    dialog->setWindowTitle("Create Parameter");
    dialog->setLayout(layout);
    dialog->setModal(true);

    QObject::connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), dialog, SLOT(reject()));

    return dialog;
}

void EvaOptimizerAdapter::setNextParameterType(const QString &type)
{
    next_type_ = type.toStdString();
}

void EvaOptimizerAdapter::pickParameter()
{
    designer_ = widget_ctrl_->getDesignerScene();
    if(designer_) {
        widget_picker_.startPicking(designer_);
    }
}

void EvaOptimizerAdapter::widgetPicked()
{
    auto node = wrapped_.lock();
    if(!node) {
        return;
    }

    QWidget* widget = widget_picker_.getWidget();
    if(widget) {
        if(widget != nullptr) {
            std::cerr << "picked widget " << widget->metaObject()->className() << std::endl;
        }

        QVariant var = widget->property("parameter");
        if(!var.isNull()) {
            csapex::param::Parameter* connected_parameter = static_cast<csapex::param::Parameter*>(var.value<void*>());

            if(connected_parameter != nullptr) {
                node->ainfo << "picked parameter " << connected_parameter->name()  << " with UUID " << connected_parameter->getUUID() << std::endl;

                csapex::param::Parameter::Ptr new_parameter = csapex::param::ParameterFactory::clone(connected_parameter);
                node->addPersistentParameter(new_parameter);

                if(!connected_parameter->isInteractive()) {
                    connected_parameter->setInteractive(true);
                }
                new_parameter->setInteractive(true);



                UUID from = UUIDProvider::makeDerivedUUID_forced(new_parameter->getUUID(), std::string("out_") + new_parameter->name());
                UUID to = UUIDProvider::makeDerivedUUID_forced(connected_parameter->getUUID(), std::string("in_") + connected_parameter->name());

                command::AddMessageConnection::Ptr cmd(new command::AddMessageConnection(from, to));

                widget_ctrl_->getCommandDispatcher()->execute(cmd);

            } else {
                node->ainfo << "no parameter selected" << std::endl;
            }
        } else {
            node->ainfo << "widget has no parameter property" << std::endl;
        }
    } else {
        node->ainfo << "no widget selected" << std::endl;
    }
}

void EvaOptimizerAdapter::createParameter()
{
    auto node = wrapped_.lock();
    if(!node) {
        return;
    }
    QDialog* type_dialog = makeTypeDialog();
    if(type_dialog->exec() == QDialog::Accepted) {

        ParameterDialog diag(next_type_);
        if(diag.exec() == QDialog::Accepted) {
            csapex::param::Parameter::Ptr param = diag.getParameter();
            node->addPersistentParameter(param);
        }
    }
}


