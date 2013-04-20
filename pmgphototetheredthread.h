#ifndef PMGPHOTOTETHEREDTHREAD_H
#define PMGPHOTOTETHEREDTHREAD_H

#include <QThread>

#include "pmgphotocommandthread.h"

class PMGPhotoTetheredThread : public QThread
{
    Q_OBJECT
public:
    explicit PMGPhotoTetheredThread(GPContext *context, PMCamera *camera);
protected:
    void run();
private:
    GPContext *context;
    PMCamera *camera;
    bool stop;
signals:
    void cameraStatus(QString message);
};

#endif // PMGPHOTOTETHEREDTHREAD_H
