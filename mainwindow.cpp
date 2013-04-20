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


    connect(&commandThread, SIGNAL(camerasDetected(QList<PMCamera*>*)), this, SLOT(camerasDetected(QList<PMCamera*>*)));
    connect(&cameraSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(cameraSelected(int)));
    connect(&commandThread, SIGNAL(cameraError(QString)), this, SLOT(displayError(QString)));
    connect(&commandThread, SIGNAL(cameraStatus(QString)), this, SLOT(displayStatus(QString)));
    connect(&commandThread, SIGNAL(liveViewStopped(int)), this, SLOT(liveViewStopped(int)));
    connect(buttonStartLiveView, SIGNAL(clicked()), this, SLOT(startLiveView()));
    connect(buttonStopLiveView, SIGNAL(clicked()), this, SLOT(stopLiveView()));



    connect(&commandThread, SIGNAL(previewAvailable(CameraFile*)), this, SLOT(displayPreview(CameraFile*)));


    commandThread.start();
    commandThread.autodetect();

    this->ui->graphicsView->setScene(new QGraphicsScene());
    this->ui->graphicsView->scene()->addItem(&preview);


    this->ui->autofocusButton->setProperty(PM_PROP_CONFIG_KEY, QVariant("autofocusdrive"));
    connect(this->ui->autofocusButton, SIGNAL(clicked()), this, SLOT(cameraSetWidgetValue()));
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
    int* value = new int();
    *value = 0;
    commandThread.setWidgetValue(cameraNumber, PM_CONFIG_KEY_VIEWFINDER, value);
    this->ui->autofocusButton->setDisabled(false);
}

void MainWindow::displayError(QString message) {
    this->ui->statusBar->showMessage(message);
}

void MainWindow::displayStatus(QString message) {
    this->ui->statusBar->showMessage(message);
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


        QPushButton* button = dynamic_cast<QPushButton*>(sender);
        if (button) {
            int* value = new int();
            *value = 1;

            commandThread.setWidgetValue(number, configKey, value);
        }
    }
}
