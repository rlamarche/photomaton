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
    OPEN_CAMERA,
    START_LIVEVIEW
};

struct PMCommand {
    PMCommandType type;
    void* args;
};

struct PMCamera {
    int cameraNumber;
    QString model;
    QString port;
    Camera *camera;
    bool liveview;
};

class PMLiveViewGPhotoThread : public QThread {
    Q_OBJECT
public:
    explicit PMLiveViewGPhotoThread(GPContext *context, PMCamera *camera);
   // ~PMGPhotoCommandThread();
    void stopNow();
protected:
    void run();
private:
    GPContext *context;
    PMCamera *camera;
    bool stop;
signals:
    void previewAvailable(CameraFile *cameraFile);
    void cameraError(QString message);
};

class PMGPhotoCommandThread : public QThread
{
    Q_OBJECT
public:
    explicit PMGPhotoCommandThread(QObject *parent = 0);
    ~PMGPhotoCommandThread();
    
    void autodetect();
    void openCamera(int cameraNumber);
    void startLiveView(int cameraNumber);

protected:
    void run();

private:
    void commandAutodetect();
    void commandOpenCamera(int cameraNumber);
    void commandStartLiveView(int cameraNumber);

    QMutex mutex;
    QWaitCondition condition;
    QQueue<PMCommand> commandQueue;
    bool abort;
    GPContext* context;
    QList<PMCamera*> *cameras;
    QList<PMLiveViewGPhotoThread*> liveviewThreads;

    CameraAbilitiesList      *abilitieslist;
    GPPortInfoList           *portinfolist;

signals:
    void camerasDetected(QList<PMCamera*>* camerasDetected);
    void cameraOpened(PMCamera *camera);
    void cameraError(QString message);
    void cameraStatus(QString message);
    void previewAvailable(CameraFile *cameraFile);
public slots:
    void liveViewError(QString message);
    void handlePreview(CameraFile *cameraFile);
};

#endif // PHOTOMATONGPHOTOCOMMANDTHREAD_H
