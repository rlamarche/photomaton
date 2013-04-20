#include "pmgphototetheredthread.h"



PMGPhotoTetheredThread::PMGPhotoTetheredThread(GPContext *context, PMCamera *camera) :
    QThread(0), context(context), camera(camera)
{
    stop = false;
}

void PMGPhotoTetheredThread::run() {
    forever {
        static int i = 0;
        int waittime = 0;
        CameraEventType type;
        void *data;

        int ret = gp_camera_wait_for_event(camera->camera, waittime, &type, &data, context);
        if (ret < GP_OK) {
            emit cameraError(tr("Problem waiting for event"), ret);
            return;
        }

        switch (type) {
        case GP_EVENT_UNKNOWN:
            emit cameraStatus(QString().sprintf("%s %d", (char*) data, i ++));
            break;
        default: break;
        }

        delete data;

        if (stop) {
            break;
        }
    }
}

void PMGPhotoTetheredThread::stopNow() {
    stop = true;
    wait();
}

void PMGPhotoTetheredThread::restart() {
    stop = false;
    start();
}
