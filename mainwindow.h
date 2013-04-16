#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pmgphotocommandthread.h"

#include <QMainWindow>
#include <QComboBox>
#include <QPixmap>
#include <QGraphicsPixmapItem>

#include <gphoto2/gphoto2-camera.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    PMGPhotoCommandThread commandThread;
    QComboBox cameraSelector;
    QGraphicsPixmapItem preview;

public slots:
    void camerasDetected(QList<PMCamera*>* cameras);
    void cameraSelected(int index);
    void startLiveView();
    void displayError(QString message);
    void displayStatus(QString message);
    void displayPreview(CameraFile *cameraFile);
};

#endif // MAINWINDOW_H
