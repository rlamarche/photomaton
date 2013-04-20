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
    void restart();
protected:
    void run();
private:
    GPContext *context;
    PMCamera *camera;
    bool stop;
    bool running;
signals:
    void previewAvailable(CameraFile *cameraFile);
    void cameraError(const QString& message, int errorCode);
    void liveViewStopped(int cameraNumber);
};


#endif // PMGPHOTOLIVEVIEWGPHOTOTHREAD_H
