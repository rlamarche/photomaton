#ifndef PMGPHOTOLIVEVIEWGPHOTOTHREAD_H
#define PMGPHOTOLIVEVIEWGPHOTOTHREAD_H

#include <QThread>

#include "pmgphotocommandthread.h"

class PMGPhotoLiveViewGPhotoThread : public QThread {
    Q_OBJECT
public:
    explicit PMGPhotoLiveViewGPhotoThread(GPContext *context, PMCamera *camera);
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


#endif // PMGPHOTOLIVEVIEWGPHOTOTHREAD_H
