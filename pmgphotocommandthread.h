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

struct PMCommand_SetWidgetValue {
    QString configKey;
    enum {
        TOGGLE_VALUE,
        RADIO_VALUE
    } valueType;
    union {
        int toggleValue;
        QString *radioValue;
    };

};


struct PMCommand {
    PMCommandType type;
    int cameraNumber;

    union {
        PMCommand_SetWidgetValue* widgetValue;
    };
};




class PMGPhotoLiveViewGPhotoThread;
class PMGPhotoTetheredThread;

struct PMCamera {
    int cameraNumber;
    QString model;
    QString port;
    Camera *camera;
    GPPort *gpport;
    PMGPhotoLiveViewGPhotoThread* liveviewThread;
    PMGPhotoTetheredThread* tetheredThread;
    CameraWidget *window;
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
    void setWidgetValue(int cameraNumber, PMCommand_SetWidgetValue *value);
    void readWidgetsValue(int cameraNumber);

protected:
    void run();
    int findWidgets(int cameraNumber, CameraWidget* widget);

private:
    void commandAutodetect();
    void commandOpenCamera(int cameraNumber);
    void commandStartLiveView(int cameraNumber);
    void commandStopLiveView(int cameraNumber);
    void commandSetWidgetValue(int cameraNumber, PMCommand_SetWidgetValue* value);

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
    void cameraError(QString message, int errorCode);
    void cameraErrorString(QString message);
    void cameraStatus(const QString& message);
    void previewAvailable(CameraFile *cameraFile);
    void liveViewStarted(int cameraNumber);
    void liveViewStopped(int cameraNumber);
    void newWidget(int cameraNumber, CameraWidget* cameraWidget);
public slots:
    void handleError(const QString& message, int errorCode);
    void handleCameraStatus(const QString& message);
    void handlePreview(CameraFile *cameraFile);
    void handleLiveViewStopped(int cameraNumber);
};

QString
pm_gp_port_result_as_string (int result);

#endif // PHOTOMATONGPHOTOCOMMANDTHREAD_H
