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
    ui->mainToolBar->addWidget(buttonStartLiveView);


    connect(&commandThread, SIGNAL(camerasDetected(QList<PMCamera*>*)), this, SLOT(camerasDetected(QList<PMCamera*>*)));
    connect(&cameraSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(cameraSelected(int)));
    connect(&commandThread, SIGNAL(cameraError(QString)), this, SLOT(displayError(QString)));
    connect(&commandThread, SIGNAL(cameraStatus(QString)), this, SLOT(displayStatus(QString)));
    connect(buttonStartLiveView, SIGNAL(clicked()), this, SLOT(startLiveView()));


    connect(&commandThread, SIGNAL(previewAvailable(CameraFile*)), this, SLOT(displayPreview(CameraFile*)));


    commandThread.start();
    commandThread.autodetect();

    this->ui->graphicsView->setScene(new QGraphicsScene());
    this->ui->graphicsView->scene()->addItem(&preview);
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
    int currentIndex = cameraSelector.currentIndex();
    if (currentIndex > 0) {
        QVariant data = cameraSelector.itemData(currentIndex);
        int number = data.toInt();
        commandThread.startLiveView(number);
    }
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
