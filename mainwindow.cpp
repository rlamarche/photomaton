#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    ui->mainToolBar->addWidget(new QLabel(tr("Appareil photo : ")));
    ui->mainToolBar->addWidget(&cameraSelector);

    QPushButton *buttonStartLiveView = new QPushButton(tr("Start LiveView"));
    QPushButton *buttonStopLiveView = new QPushButton(tr("Stop LiveView"));
    ui->mainToolBar->addWidget(buttonStartLiveView);
    ui->mainToolBar->addWidget(buttonStopLiveView);

    logModel = new QStandardItemModel(this);
    logModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Date/Heure")));
    logModel->setHorizontalHeaderItem(1, new QStandardItem(tr("Message")));

    ui->treeView->setModel(logModel);

    ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    connect(&commandThread, SIGNAL(camerasDetected(QList<PMCamera*>*)), this, SLOT(camerasDetected(QList<PMCamera*>*)));
    connect(&cameraSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(cameraSelected(int)));
    connect(&commandThread, SIGNAL(cameraErrorString(QString)), this, SLOT(displayError(QString)));
    connect(&commandThread, SIGNAL(cameraStatus(QString)), this, SLOT(displayStatus(QString)));
    connect(&commandThread, SIGNAL(liveViewStopped(int)), this, SLOT(liveViewStopped(int)));
    connect(&commandThread, SIGNAL(newWidget(int,CameraWidget*)), this, SLOT(newWidget(int,CameraWidget*)));

    connect(buttonStartLiveView, SIGNAL(clicked()), this, SLOT(startLiveView()));
    connect(buttonStopLiveView, SIGNAL(clicked()), this, SLOT(stopLiveView()));



    connect(&commandThread, SIGNAL(previewAvailable(CameraFile*)), this, SLOT(displayPreview(CameraFile*)));


    commandThread.start();
    commandThread.autodetect();

    this->ui->graphicsView->setScene(new QGraphicsScene());
    this->ui->graphicsView->scene()->addItem(&preview);


    cameraWidgets[PM_CONFIG_KEY_AUTOFOCUS_DRIVE] = this->ui->autofocusButton;
    cameraWidgets[PM_CONFIG_KEY_F_NUMBER] = this->ui->apertureComboBox;
    cameraWidgets[PM_CONFIG_KEY_SHUTTERSPEED] = this->ui->shutterspeedComboBox;

    configureWidgets();
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
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::camerasDetected(QList<PMCamera*> *cameras) {
    cameraSelector.clear();

    QList<PMCamera*>::iterator i;

    QString label = QString(tr("%1 appareils photo détecté(s)")).arg(QString::number(cameras->length()));
    cameraSelector.addItem(label);

    for (i = cameras->begin(); i != cameras->end(); ++i) {
        cameraSelector.addItem((*i)->model, (*i)->cameraNumber);
    }

}

void MainWindow::cameraSelected(int index) {
    if (index > 0) {
        QVariant data = cameraSelector.itemData(index);
        int number = data.toInt();
        commandThread.openCamera(number);
    }


}

void MainWindow::startLiveView() {
    this->ui->autofocusButton->setDisabled(true);
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();
        commandThread.startLiveView(number);
    }
}

void MainWindow::stopLiveView() {
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();
        commandThread.stopLiveView(number);
    }
}

void MainWindow::liveViewStopped(int cameraNumber) {

    PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
    value->configKey = PM_CONFIG_KEY_VIEWFINDER;
    value->valueType = PMCommand_SetWidgetValue::TOGGLE_VALUE;
    value->toggleValue = 0;

    commandThread.setWidgetValue(cameraNumber, value);
    this->ui->autofocusButton->setDisabled(false);
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

    ui->treeView->scrollTo(logModel->index(logModel->rowCount() - 1, 0));
}

void MainWindow::displayPreview(CameraFile *cameraFile) {
    static QTime time;
    static int frameCount = 0;
    static QGraphicsTextItem fpsText;

    if (frameCount == 0) {
        time.start();
        this->ui->graphicsView->scene()->addItem(&fpsText);
    }

    frameCount ++;

    unsigned long int size;
    const char *data;
    QPixmap pixmap;

    gp_file_get_data_and_size(cameraFile, &data, &size);

    pixmap.loadFromData((uchar*) data, size, "JPG");
    preview.setPixmap(pixmap);

    gp_file_free(cameraFile);

    float fps = time.elapsed() / float(frameCount);
    fpsText.setPlainText(QString("FPS : %1").arg(fps));

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

            commandThread.setWidgetValue(number, value);
        }

        QComboBox *comboBox = dynamic_cast<QComboBox*>(sender);
        if (comboBox) {
            PMCommand_SetWidgetValue *value = new PMCommand_SetWidgetValue;
            value->configKey = configKey;
            value->valueType = PMCommand_SetWidgetValue::RADIO_VALUE;
            value->radioValue = new QString(comboBox->itemData(comboBox->currentIndex()).toString());

            commandThread.setWidgetValue(number, value);
        }
    }
}

void MainWindow::newWidget(int cameraNumber, CameraWidget *cameraWidget) {
    const char* label;
    const char* name;
    const char* info;
    int id, ret;
    ret = gp_widget_get_label(cameraWidget, &label);
    ret = gp_widget_get_name(cameraWidget, &name);
    ret = gp_widget_get_id(cameraWidget, &id);
    ret = gp_widget_get_info(cameraWidget, &info);

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
    }

    //addMessage(tr("New widget : %1 %2 %3 %4").arg(QString().sprintf("%d", id), QString(name), QString(label), QString(info)));

}
