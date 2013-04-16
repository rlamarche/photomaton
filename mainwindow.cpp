#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cameraSelector.setSizeAdjustPolicy(QComboBox::AdjustToContents);

    ui->mainToolBar->addWidget(new QLabel(tr("Appareil photo : ")));
    ui->mainToolBar->addWidget(&cameraSelector);

    connect(&commandThread, SIGNAL(camerasDetected(QList<PMCamera>*)), this, SLOT(camerasDetected(QList<PMCamera>*)));
    connect(&cameraSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(cameraSelected(int)));

    commandThread.start();
    commandThread.autodetect();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::camerasDetected(QList<PMCamera> *cameras) {
    cameraSelector.clear();

    QList<PMCamera>::iterator i;

    QString label = QString(tr("%1 appareils photo détecté(s)")).arg(QString::number(cameras->length()));
    cameraSelector.addItem(label);

    for (i = cameras->begin(); i != cameras->end(); ++i) {
        cameraSelector.addItem((*i).name, (*i).cameraNumber);
    }

}

void MainWindow::cameraSelected(int index) {
    if (index > 0) {
        QVariant data = cameraSelector.itemData(index);
        int number = data.toInt();

    }
}
