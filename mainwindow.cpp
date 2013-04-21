#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pmslider.h"

#include <QLabel>
#include <QComboBox>

#include <QPushButton>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cameraSelector.setSizeAdjustPolicy(QComboBox::AdjustToContents);

    ui->mainToolBar->addWidget(new QLabel(tr("Camera : ")));
    ui->mainToolBar->addWidget(&cameraSelector);

    QPushButton *buttonStartLiveView = new QPushButton(tr("Start LiveView"));
    QPushButton *buttonStopLiveView = new QPushButton(tr("Stop LiveView"));
    ui->mainToolBar->addWidget(buttonStartLiveView);
    ui->mainToolBar->addWidget(buttonStopLiveView);

    logModel = new QStandardItemModel(this);
    logModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Date/Time")));
    logModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Message")));

    ui->messagesTreeView->setModel(logModel);

    ui->messagesTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);


    infosModel = new QStandardItemModel(this);
    infosModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Label")));
    infosModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Name")));
    infosModel->setHorizontalHeaderItem(2, new QStandardItem(tr("Value")));


    ui->infosTreeView->setModel(infosModel);
    ui->infosTreeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->infosTreeView->header()->hideSection(1);


    commandThread = new PMGPhotoCommandThread(this);

    connect(commandThread, SIGNAL(camerasDetected(QList<PMCamera*>*)), this, SLOT(camerasDetected(QList<PMCamera*>*)));
    connect(&cameraSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(cameraSelected(int)));
    connect(commandThread, SIGNAL(cameraErrorString(QString)), this, SLOT(displayError(QString)));
    connect(commandThread, SIGNAL(cameraStatus(QString)), this, SLOT(displayStatus(QString)));
    connect(commandThread, SIGNAL(liveViewStopped(int)), this, SLOT(liveViewStopped(int)));
    connect(commandThread, SIGNAL(newWidget(int,CameraWidget*)), this, SLOT(newWidget(int,CameraWidget*)));
    connect(commandThread, SIGNAL(cameraOpened(int)), this, SLOT(cameraOpened(int)));

    connect(buttonStartLiveView, SIGNAL(clicked()), this, SLOT(startLiveView()));
    connect(buttonStopLiveView, SIGNAL(clicked()), this, SLOT(stopLiveView()));



    connect(commandThread, SIGNAL(previewAvailable(CameraFile*)), ui->graphicsView, SLOT(displayPreview(CameraFile*)));


    commandThread->start();
    commandThread->autodetect();

    cameraWidgets[PM_CONFIG_KEY_AUTOFOCUS_DRIVE] = this->ui->autofocusButton;
    cameraWidgets[PM_CONFIG_KEY_F_NUMBER] = this->ui->apertureComboBox;
    cameraWidgets[PM_CONFIG_KEY_SHUTTERSPEED] = this->ui->shutterspeedComboBox;
    //cameraWidgets[PM_CONFIG_KEY_MANUALFOCUSDRIVE] = this->ui->manualFocusSlider;

    configureWidgets();

    ui->mTenToolButton->setProperty(PM_PROP_CONFIG_KEY, QVariant(QString(PM_CONFIG_KEY_MANUALFOCUSDRIVE)));
    ui->mFiveToolButton->setProperty(PM_PROP_CONFIG_KEY, QVariant(QString(PM_CONFIG_KEY_MANUALFOCUSDRIVE)));
    ui->mOneToolButton->setProperty(PM_PROP_CONFIG_KEY, QVariant(QString(PM_CONFIG_KEY_MANUALFOCUSDRIVE)));
    ui->pOneToolButton->setProperty(PM_PROP_CONFIG_KEY, QVariant(QString(PM_CONFIG_KEY_MANUALFOCUSDRIVE)));
    ui->pFiveToolButton->setProperty(PM_PROP_CONFIG_KEY, QVariant(QString(PM_CONFIG_KEY_MANUALFOCUSDRIVE)));
    ui->pTenToolButton->setProperty(PM_PROP_CONFIG_KEY, QVariant(QString(PM_CONFIG_KEY_MANUALFOCUSDRIVE)));

    ui->mTenToolButton->setProperty(PM_PROP_OFFSET, QVariant(-1000));
    ui->mFiveToolButton->setProperty(PM_PROP_OFFSET, QVariant(-100));
    ui->mOneToolButton->setProperty(PM_PROP_OFFSET, QVariant(-1));
    ui->pOneToolButton->setProperty(PM_PROP_OFFSET, QVariant(1));
    ui->pFiveToolButton->setProperty(PM_PROP_OFFSET, QVariant(100));
    ui->pTenToolButton->setProperty(PM_PROP_OFFSET, QVariant(1000));

    connect(ui->mTenToolButton, SIGNAL(clicked()), this, SLOT(manualFocusDrive()));
    connect(ui->mFiveToolButton, SIGNAL(clicked()), this, SLOT(manualFocusDrive()));
    connect(ui->mOneToolButton, SIGNAL(clicked()), this, SLOT(manualFocusDrive()));
    connect(ui->pOneToolButton, SIGNAL(clicked()), this, SLOT(manualFocusDrive()));
    connect(ui->pFiveToolButton, SIGNAL(clicked()), this, SLOT(manualFocusDrive()));
    connect(ui->pTenToolButton, SIGNAL(clicked()), this, SLOT(manualFocusDrive()));

}

void MainWindow::configureWidgets() {
    QList<QString>::iterator i;
    QList<QString> keys = cameraWidgets.keys();

    for (i = keys.begin(); i != keys.end(); ++ i) {
        QWidget* widget = cameraWidgets[*i];
        widget->setDisabled(true);

        widget->setProperty(PM_PROP_CONFIG_KEY, QVariant(*i));

        QComboBox *comboBox = dynamic_cast<QComboBox*>(widget);
        if (comboBox) {
            connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(cameraSetWidgetValue()));
        }
        QPushButton *pushButton = dynamic_cast<QPushButton*>(widget);
        if (pushButton) {
            connect(pushButton, SIGNAL(clicked()), this, SLOT(cameraSetWidgetValue()));

        }
        PMSlider *slider = dynamic_cast<PMSlider*>(widget);
        if (slider) {
            connect(slider, SIGNAL(sliderReleased()), this, SLOT(cameraSetWidgetValue()));
            connect(slider, SIGNAL(keyReleased()), this, SLOT(cameraSetWidgetValue()));
        }
    }
}

MainWindow::~MainWindow()
{
    delete commandThread;
    delete ui;
}

void MainWindow::camerasDetected(QList<PMCamera*> *cameras) {
    cameraSelector.clear();

    QList<PMCamera*>::iterator i;

    QString label = QString(tr("%1 detected camera(s)")).arg(QString::number(cameras->length()));
    cameraSelector.addItem(label);

    for (i = cameras->begin(); i != cameras->end(); ++i) {
        cameraSelector.addItem((*i)->model, (*i)->cameraNumber);
    }

}

void MainWindow::cameraSelected(int index) {
    if (index > 0) {
        QVariant data = cameraSelector.itemData(index);
        int number = data.toInt();
        commandThread->openCamera(number);
    }


}

void MainWindow::cameraOpened(int cameraNumber) {
    // manual focus works only in liveview, so disable on init
    //ui->manualFocusSlider->setDisabled(true);
    ui->infosTreeView->expandAll();
}

void MainWindow::startLiveView() {
    this->ui->autofocusButton->setDisabled(true);
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();

        PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
        value->configKey = PM_CONFIG_KEY_VIEWFINDER;
        value->valueType = PMCommand_SetWidgetValue::TOGGLE_VALUE;
        value->toggleValue = 1;

        commandThread->setWidgetValue(number, value);


        commandThread->startLiveView(number);
        //this->ui->manualFocusSlider->setDisabled(false);
    }
}

void MainWindow::stopLiveView() {
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();
        commandThread->stopLiveView(number);
    }


}

void MainWindow::liveViewStopped(int cameraNumber) {

    //commandThread->updateWidgets(cameraNumber);

    PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
    value->configKey = PM_CONFIG_KEY_VIEWFINDER;
    value->valueType = PMCommand_SetWidgetValue::TOGGLE_VALUE;
    value->toggleValue = 0;

    commandThread->setWidgetValue(cameraNumber, value);
    this->ui->autofocusButton->setDisabled(false);
    //this->ui->manualFocusSlider->setDisabled(true);

}

void MainWindow::displayError(QString message) {
    this->ui->statusBar->showMessage(message);

    addMessage(message);
}

void MainWindow::displayStatus(QString message) {
    this->ui->statusBar->showMessage(message);

    addMessage(message);
}

void MainWindow::addMessage(QString message) {
    QList<QStandardItem *> list;

    QDateTime currentDateTime = QDateTime::currentDateTime();

    list << new QStandardItem(currentDateTime.toString(Qt::SystemLocaleShortDate).append(":").append(currentDateTime.toString("ss")));
    list << new QStandardItem(message);

    logModel->appendRow(list);

    ui->messagesTreeView->scrollTo(logModel->index(logModel->rowCount() - 1, 0));
}

void MainWindow::manualFocusDrive() {
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();

        QObject* sender = QObject::sender();
        QToolButton *toolButton = dynamic_cast<QToolButton*>(sender);
        if (toolButton) {
            int offset = toolButton->property(PM_PROP_OFFSET).toInt();


            PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
            value->configKey = PM_CONFIG_KEY_MANUALFOCUSDRIVE;
            value->valueType = PMCommand_SetWidgetValue::RANGE_VALUE;
            value->rangeValue = 0;

            commandThread->setWidgetValue(number, value);

            value = new PMCommand_SetWidgetValue;
            value->configKey = PM_CONFIG_KEY_MANUALFOCUSDRIVE;
            value->valueType = PMCommand_SetWidgetValue::RANGE_VALUE;
            value->rangeValue = offset;

            commandThread->setWidgetValue(number, value);
        }
    }
}

void MainWindow::cameraSetWidgetValue() {
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();
        QObject* sender = QObject::sender();

        const QString& configKey = sender->property(PM_PROP_CONFIG_KEY).toString();


        QPushButton *button = dynamic_cast<QPushButton*>(sender);
        if (button) {
            PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
            value->configKey = configKey;
            value->valueType = PMCommand_SetWidgetValue::TOGGLE_VALUE;
            value->toggleValue = 1;

            commandThread->setWidgetValue(number, value);
        }

        QComboBox *comboBox = dynamic_cast<QComboBox*>(sender);
        if (comboBox) {
            PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
            value->configKey = configKey;
            value->valueType = PMCommand_SetWidgetValue::RADIO_VALUE;
            value->radioValue = new QString(comboBox->itemData(comboBox->currentIndex()).toString());

            commandThread->setWidgetValue(number, value);
        }
        QSlider *slider = dynamic_cast<QSlider*>(sender);
        if (slider) {
            PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
            value->configKey = configKey;
            value->valueType = PMCommand_SetWidgetValue::RANGE_VALUE;
            value->rangeValue = (float) slider->value();

            commandThread->setWidgetValue(number, value);

        }
    }
}

void MainWindow::startListingWidgets(int cameraNumber) {
    infosModel->clear();
}

void MainWindow::newWidget(int cameraNumber, CameraWidget *cameraWidget) {
    CameraWidget* parent;
    const char* label;
    const char* name;
    const char* parentName;
    const char* info;
    CameraWidgetType type;
    int id, ret;
    ret = gp_widget_get_label(cameraWidget, &label);
    ret = gp_widget_get_name(cameraWidget, &name);
    ret = gp_widget_get_id(cameraWidget, &id);
    ret = gp_widget_get_info(cameraWidget, &info);
    ret = gp_widget_get_type(cameraWidget, &type);
    ret = gp_widget_get_parent(cameraWidget, &parent);
    ret = gp_widget_get_name(parent, &parentName);

    QStandardItem *parentItem = 0;

    QString parentNameStr(parentName);
    for (int i = 0; i < infosModel->invisibleRootItem()->rowCount(); ++ i) {
        QStandardItem *currentItem = infosModel->invisibleRootItem()->child(i, 1);
        if (currentItem->text() == parentNameStr) {
            parentItem = infosModel->invisibleRootItem()->child(i, 0);
        }
    }

    // Fill infos tab
    QList<QStandardItem *> list;

    //QDateTime currentDateTime = QDateTime::currentDateTime();

    list << new QStandardItem(label);
    list << new QStandardItem(name);

    const char* strValue;
    int intValue;
    float floatValue;

    switch (type) {
    case GP_WIDGET_WINDOW:
        break;
    case GP_WIDGET_SECTION:
        break;
    case GP_WIDGET_BUTTON:
        break;
    case GP_WIDGET_TOGGLE:
        gp_widget_get_value(cameraWidget, &intValue);
        list << new QStandardItem(QString().sprintf("%d", intValue));
        break;
    case GP_WIDGET_RANGE:
        gp_widget_get_value(cameraWidget, &floatValue);
        list << new QStandardItem(QString().sprintf("%f", floatValue));
        break;
    case GP_WIDGET_RADIO:
    case GP_WIDGET_TEXT:
    case GP_WIDGET_MENU:
        gp_widget_get_value(cameraWidget, &strValue);
        list << new QStandardItem(strValue);
        break;
    case GP_WIDGET_DATE:
        gp_widget_get_value(cameraWidget, &intValue);
        list << new QStandardItem(QString("%1").arg(QDateTime::fromTime_t(intValue).toString(Qt::SystemLocaleLongDate)));
        break;
    }
    QString widgetNameStr(name);
    if (parentItem) {
       /* QStandardItem *valueItem;
        for (int i = 0; i < parentItem->rowCount(); i ++) {
            QStandardItem *item = parentItem->child(i, 1);
            if (item->text() == widgetNameStr) {

            }
        }*/
        parentItem->appendRow(list);
    } else {
        if (type == GP_WIDGET_SECTION) {
            infosModel->invisibleRootItem()->appendRow(list);
        } else {
           // infosModel->appendRow(list);
        }
    }


    QWidget* widget = cameraWidgets[QString(name)];
    if (widget) {
        QComboBox *comboBox = dynamic_cast<QComboBox*>(widget);
        if (comboBox) {
            comboBox->blockSignals(true);
            comboBox->clear();
            char* currentValue;
            const char* choiceLabel;
            gp_widget_get_value(cameraWidget, &currentValue);
            QString currentValueStr(currentValue);

            int n = gp_widget_count_choices(cameraWidget);

            for (int i = 0; i < n; i ++) {
                ret = gp_widget_get_choice(cameraWidget, i, &choiceLabel);
                QString choiceLabelStr(choiceLabel);

                comboBox->addItem(choiceLabelStr, QVariant(choiceLabelStr));
                if (currentValueStr == choiceLabelStr) {
                    comboBox->setCurrentIndex(i);
                }
            }
            comboBox->blockSignals(false);
            comboBox->setDisabled(false);
        }
        QPushButton *pushButton = dynamic_cast<QPushButton*>(widget);
        if (pushButton) {
            pushButton->setDisabled(false);
        }

        QSlider *slider = dynamic_cast<QSlider*>(widget);
        if (slider) {
            slider->blockSignals(true);
            float min, max, increment;
            float value;

            ret = gp_widget_get_range(cameraWidget, &min, &max, &increment);
            ret = gp_widget_get_value(cameraWidget, &value);

            slider->setMinimum((int) min);
            slider->setMaximum((int) max);
            slider->setSingleStep((int) increment);
            slider->setPageStep((int)increment * 10);


            slider->setValue((int)  value);

            slider->blockSignals(false);
            slider->setDisabled(false);
        }
    }

    //addMessage(tr("New widget : %1 %2 %3 %4").arg(QString().sprintf("%d", id), QString(name), QString(label), QString(info)));

}
