#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pmgphotocommandthread.h"

#include <QMainWindow>
#include <QComboBox>
#include <QPixmap>
#include <QGraphicsPixmapItem>

#include <gphoto2/gphoto2-camera.h>

#define PM_PROP_CONFIG_KEY "configKey"


#define PM_CONFIG_KEY_AUTOFOCUS_DRIVE "autofocusdrive"
#define PM_CONFIG_KEY_VIEWFINDER "viewfinder"

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
    void stopLiveView();
    void liveViewStopped(int cameraNumber);
    void displayError(QString message);
    void displayStatus(QString message);
    void displayPreview(CameraFile *cameraFile);
    void cameraSetWidgetValue();
};

#endif // MAINWINDOW_H
