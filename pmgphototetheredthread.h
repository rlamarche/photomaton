#ifndef PMGPHOTOTETHEREDTHREAD_H
#define PMGPHOTOTETHEREDTHREAD_H

#include <QThread>

#include "pmgphotocommandthread.h"

class PMGPhotoTetheredThread : public QThread
{
    Q_OBJECT
public:
    explicit PMGPhotoTetheredThread(GPContext *context, PMCamera *camera);
    void stopNow();
    void restart();
protected:
    void run();
private:
    GPContext *context;
    PMCamera *camera;
    bool stop;
signals:
    void cameraStatus(const QString& message);
    void cameraError(const QString& message, int errorCode);
};

#endif // PMGPHOTOTETHEREDTHREAD_H
