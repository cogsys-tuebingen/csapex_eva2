/// HEADER
#include "eva_optimizer_adapter.h"

/// COMPONENT
#include <csapex_optimization/parameter_dialog.h>

/// PROJECT
#include <csapex/utility/register_node_adapter.h>
#include <utils_param/range_parameter.h>
#include <csapex/view/widget_controller.h>
#include <csapex/view/designer_scene.h>
#include <utils_param/parameter_factory.h>

/// SYSTEM
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFormLayout>
#include <QtConcurrentRun>
#include <QFutureWatcher>

using namespace csapex;

CSAPEX_REGISTER_NODE_ADAPTER(EvaOptimizerAdapter, csapex::EvaOptimizer)


EvaOptimizerAdapter::EvaOptimizerAdapter(NodeWorker* worker, EvaOptimizer *node, WidgetController* widget_ctrl)
    : DefaultNodeAdapter(worker, widget_ctrl), wrapped_(node), designer_(widget_ctrl_->getDesignerScene())
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


    QPushButton* btn_start_optimization = new QPushButton("start");
    layout->addWidget(btn_start_optimization);
    QObject::connect(btn_start_optimization, SIGNAL(clicked()), this, SLOT(startOptimization()));


    QPushButton* btn_stop_optimization = new QPushButton("stop");
    layout->addWidget(btn_stop_optimization);
    QObject::connect(btn_stop_optimization, SIGNAL(clicked()), this, SLOT(stopOptimization()));
}

void EvaOptimizerAdapter::startOptimization()
{
    wrapped_->start();
}


void EvaOptimizerAdapter::stopOptimization()
{
    wrapped_->stop();
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
    QWidget* widget = widget_picker_.getWidget();
    if(widget) {
        std::cout << "selected: " << widget->metaObject()->className() << std::endl;
        QVariant var = widget->property("parameter");
        if(!var.isNull()) {
            param::Parameter* p = static_cast<param::Parameter*>(var.value<void*>());

            if(p != NULL) {
                node_->getNode()->ainfo << "picked parameter " << p->name() << std::endl;

                wrapped_->addPersistentParameter(param::ParameterFactory::clone(p));
                return;
            }
        }
    }
    node_->getNode()->ainfo << "no parameter selected" << std::endl;
}

void EvaOptimizerAdapter::createParameter()
{
    QDialog* type_dialog = makeTypeDialog();
    if(type_dialog->exec() == QDialog::Accepted) {

        ParameterDialog diag(next_type_);
        if(diag.exec() == QDialog::Accepted) {
            param::Parameter::Ptr param = diag.getParameter();
            wrapped_->addPersistentParameter(param);
        }
    }
}


