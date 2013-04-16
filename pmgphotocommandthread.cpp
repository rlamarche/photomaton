#include "pmgphotocommandthread.h"

#include <QVector>

#include <iostream>

PMGPhotoCommandThread::PMGPhotoCommandThread(QObject *parent) :
    QThread(parent)
{
    abort = false;
    detectedCameras = 0;
    context = 0;
}
PMGPhotoCommandThread::~PMGPhotoCommandThread() {
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();

    if (detectedCameras) {
        delete detectedCameras;
    }
    if (context) {
        gp_context_unref(context);
    }
}

void PMGPhotoCommandThread::run() {
    forever {
        if (abort) {
            return;
        }
        PMCommand command;
        bool empty;


        mutex.lock();
        if (!commandQueue.isEmpty()) {
            empty = false;
            command = commandQueue.head();
        } else {
            empty = true;
        }
        mutex.unlock();

        if (!empty) {
            switch (command.type) {
                case AUTODETECT_CAMERAS:
                    commandAutodetect();
                break;
                case OPEN_CAMERA:
                commandOpenCamera(*((int*) command.args));
                break;
            }
        }

        mutex.lock();
        condition.wait(&mutex);
        mutex.unlock();
    }
}

void PMGPhotoCommandThread::autodetect() {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = AUTODETECT_CAMERAS;

    commandQueue.append(command);
}

void PMGPhotoCommandThread::commandAutodetect() {
    int count;
    CameraList *list;
    const char *name = NULL, *value = NULL;

    if (!context) {
        context = gp_context_new();
    }

    gp_list_new (&list);
    count = gp_camera_autodetect(list, context);


    if (detectedCameras) {
        delete detectedCameras;
    }
    detectedCameras = new QList<PMCamera>();

    std::cout << count << "\n";
    for (int i = 0; i < count; i ++) {
        gp_list_get_name  (list, i, &name);
        gp_list_get_value (list, i, &value);

        PMCamera camera;
        camera.name = QString(name);
        camera.value = QString(value);
        camera.cameraNumber = i;

        detectedCameras->append(camera);
    }

    gp_list_free(list);

    emit camerasDetected(detectedCameras);
}

void PMGPhotoCommandThread::commandOpenCamera(int cameraNumber) {

}
