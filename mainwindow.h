#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pmgphotocommandthread.h"

#include <QMainWindow>
#include <QComboBox>

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

public slots:
    void camerasDetected(QList<PMCamera>* cameras);
    void cameraSelected(int index);
};

#endif // MAINWINDOW_H
