#ifndef PHOTOMATONGPHOTOCOMMANDTHREAD_H
#define PHOTOMATONGPHOTOCOMMANDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QList>
#include <QMap>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-port-info-list.h>

enum PMCommandType {
    AUTODETECT_CAMERAS,
    OPEN_CAMERA
};

struct PMCommand {
    PMCommandType type;
    void* args;
};

struct PMCamera {
    int cameraNumber;
    QString name;
    QString value;
};

class PMGPhotoCommandThread : public QThread
{
    Q_OBJECT
public:
    explicit PMGPhotoCommandThread(QObject *parent = 0);
    ~PMGPhotoCommandThread();
    
    void autodetect();
    void openCamera(int cameraNumber);

protected:
    void run();

private:
    void commandAutodetect();
    void commandOpenCamera(int cameraNumber);

    QMutex mutex;
    QWaitCondition condition;
    QQueue<PMCommand> commandQueue;
    bool abort;
    GPContext* context;
    QList<PMCamera> *detectedCameras;

signals:
    void camerasDetected(QList<PMCamera>* camerasDetected);
public slots:
    
};

#endif // PHOTOMATONGPHOTOCOMMANDTHREAD_H
