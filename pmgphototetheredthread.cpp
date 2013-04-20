#include "pmgphototetheredthread.h"

PMGPhotoTetheredThread::PMGPhotoTetheredThread(GPContext *context, PMCamera *camera) :
    QThread(0), context(context), camera(camera)
{
    stop = false;
}

void PMGPhotoTetheredThread::run() {
    forever {
        int waittime = 2000;
        CameraEventType type;
        void *data;

        int ret = gp_camera_wait_for_event(camera->camera, waittime, &type, &data, context);


        switch (type) {
        case GP_EVENT_UNKNOWN:
            emit cameraStatus(QString((char*) data));
            break;
        default: break;
        }
    }
}
