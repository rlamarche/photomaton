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
    START_LIVEVIEW,
    STOP_LIVEVIEW,
    SET_WIDGET_VALUE
};

struct PMCommand {
    PMCommandType type;
    void* args;
};



struct PMCommand_SetWidgetValue_Args {
    int cameraNumber;
    QString configKey;
    void *value;
};

class PMGPhotoLiveViewGPhotoThread;
class PMGPhotoTetheredThread;

struct PMCamera {
    int cameraNumber;
    QString model;
    QString port;
    Camera *camera;
    PMGPhotoLiveViewGPhotoThread* liveviewThread;
    PMGPhotoTetheredThread* tetheredThread;
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
    void stopLiveView(int cameraNumber);
    void setWidgetValue(int cameraNumber, const QString& configKey, const void* value);

protected:
    void run();

private:
    void commandAutodetect();
    void commandOpenCamera(int cameraNumber);
    void commandStartLiveView(int cameraNumber);
    void commandStopLiveView(int cameraNumber);
    void commandSetWidgetValue(int cameraNumber, const QString& configKey, const void* value);

    QMutex mutex;
    QWaitCondition condition;
    QQueue<PMCommand> commandQueue;
    bool abort;
    GPContext* context;
    QList<PMCamera*> *cameras;

    CameraAbilitiesList      *abilitieslist;
    GPPortInfoList           *portinfolist;

signals:
    void camerasDetected(QList<PMCamera*>* camerasDetected);
    void cameraOpened(PMCamera *camera);
    void cameraError(QString message);
    void cameraStatus(QString message);
    void previewAvailable(CameraFile *cameraFile);
    void liveViewStarted(int cameraNumber);
    void liveViewStopped(int cameraNumber);
public slots:
    void liveViewError(QString message);
    void handlePreview(CameraFile *cameraFile);
};

#endif // PHOTOMATONGPHOTOCOMMANDTHREAD_H
