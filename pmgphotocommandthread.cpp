#include "pmgphotocommandthread.h"
#include "pmgphotoliveviewgphotothread.h"

#include <QMessageBox>

#include <iostream>

PMGPhotoCommandThread::PMGPhotoCommandThread(QObject *parent) :
    QThread(parent)
{
    abort = false;
    cameras = 0;
    context = 0;
    abilitieslist = 0;
    portinfolist = 0;
}


PMGPhotoCommandThread::~PMGPhotoCommandThread() {
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();

    if (cameras) {
        QList<PMCamera*>::iterator i;
        for (i = cameras->begin(); i != cameras->end(); ++i) {
            PMCamera* camera = *i;

            if (camera->liveviewThread) {
                camera->liveviewThread->stopNow();
                delete camera->liveviewThread;
            }
            gp_camera_unref(camera->camera);
            delete camera;
        }

        delete cameras;
    }

    if (abilitieslist) {
        gp_abilities_list_free(abilitieslist);
    }

    if (portinfolist) {
        gp_port_info_list_free(portinfolist);
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
            command = commandQueue.dequeue();
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
                delete (int*) command.args;
                break;
            case START_LIVEVIEW:
                commandStartLiveView(*((int*) command.args));
                delete (int*) command.args;
                break;
            case STOP_LIVEVIEW:
                commandStopLiveView(*((int*) command.args));
                delete (int*) command.args;
                break;
            case SET_WIDGET_VALUE:
                PMCommand_SetWidgetValue_Args *args = (PMCommand_SetWidgetValue_Args*) command.args;
                commandSetWidgetValue(args->cameraNumber, args->configKey, args->value);
                delete args;
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
    condition.wakeOne();
}

void PMGPhotoCommandThread::openCamera(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = OPEN_CAMERA;
    int* arg = new int;
    *arg = cameraNumber;
    command.args = arg;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::startLiveView(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = START_LIVEVIEW;
    int* arg = new int;
    *arg = cameraNumber;
    command.args = arg;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::stopLiveView(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = STOP_LIVEVIEW;
    int* arg = new int;
    *arg = cameraNumber;
    command.args = arg;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::setWidgetValue(int cameraNumber, const QString &configKey, const void* value) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    PMCommand_SetWidgetValue_Args *args = new PMCommand_SetWidgetValue_Args();

    args->cameraNumber = cameraNumber;
    args->configKey = configKey;
    args->value = (void*) value;

    command.type = SET_WIDGET_VALUE;
    command.args = args;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::commandAutodetect() {
    int count;
    CameraList *list;
    const char *name = NULL, *value = NULL;

    if (!context) {
        context = gp_context_new();
    }

    if (abilitieslist) {
        gp_abilities_list_free(abilitieslist);
    }
    gp_abilities_list_new    (&abilitieslist);
    gp_abilities_list_load(abilitieslist, context);

    if (portinfolist) {
        gp_port_info_list_free(portinfolist);
    }
    gp_port_info_list_new(&portinfolist);
    gp_port_info_list_load(portinfolist);

    gp_list_new (&list);
    count = gp_camera_autodetect(list, context);


    if (cameras) {
        delete cameras;
    }
    cameras = new QList<PMCamera*>();

    for (int i = 0; i < count; i ++) {
        gp_list_get_name  (list, i, &name);
        gp_list_get_value (list, i, &value);

        PMCamera* camera = new PMCamera;
        camera->model = QString(name);
        camera->port = QString(value);
        camera->cameraNumber = i;
        camera->liveviewThread = 0;

        cameras->append(camera);
    }

    gp_list_free(list);

    emit camerasDetected(cameras);
}

static int _lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) {
    int ret;
    ret = gp_widget_get_child_by_name (widget, key, child);
    if (ret < GP_OK)
        ret = gp_widget_get_child_by_label (widget, key, child);
    return ret;
}

void PMGPhotoCommandThread::commandOpenCamera(int cameraNumber) {
    CameraAbilities a;
    GPPortInfo      pi;
    PMCamera* camera = cameras->at(cameraNumber);


    //int gp_abilities_list_free   (CameraAbilitiesList *list);


    int ret = gp_camera_new (& camera->camera);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to create camera")));
        return;
    }

    /* First lookup the model / driver */
    int m = gp_abilities_list_lookup_model (abilitieslist, camera->model.toStdString().c_str());
    if (m < GP_OK) {
        emit cameraError(QString(tr("Unable to lookup model")));
        return;
    }
    ret = gp_abilities_list_get_abilities (abilitieslist, m, &a);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get abilities list")));
        return;
    }
    ret = gp_camera_set_abilities (camera->camera, a);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set abilities")));
        return;
    }

    /* Then associate the camera with the specified port */
    int p = gp_port_info_list_lookup_path (portinfolist, camera->port.toStdString().c_str());

    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to lookup port")));
        return;
    }
    ret = gp_port_info_list_get_info (portinfolist, p, &pi);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get info on port")));
        return;
    }
    ret = gp_camera_set_port_info (camera->camera, pi);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set port info on camera")));
        return;
    }
    emit cameraOpened(camera);
    emit cameraStatus(QString(tr("Camera successfuly open")));
}

void PMGPhotoCommandThread::commandStartLiveView(int cameraNumber) {
    PMCamera *camera = cameras->at(cameraNumber);

    if (!camera->liveviewThread) {
        PMGPhotoLiveViewGPhotoThread *liveviewthread = new PMGPhotoLiveViewGPhotoThread(context, cameras->at(cameraNumber));
        camera->liveviewThread = liveviewthread;

        connect(liveviewthread, SIGNAL(cameraError(QString)), this, SLOT(liveViewError(QString)));
        connect(liveviewthread, SIGNAL(previewAvailable(CameraFile*)), this, SLOT(handlePreview(CameraFile*)));

        liveviewthread->start();
        emit liveViewStarted(cameraNumber);
    }
}

void PMGPhotoCommandThread::commandStopLiveView(int cameraNumber) {
    PMCamera *camera = cameras->at(cameraNumber);

    if (camera->liveviewThread) {
        camera->liveviewThread->stopNow();
        delete camera->liveviewThread;
        camera->liveviewThread = 0;
        emit liveViewStopped(cameraNumber);
    }
}

void PMGPhotoCommandThread::commandSetWidgetValue(int cameraNumber, const QString &configKey, const void* value) {
    PMCamera *camera = cameras->at(cameraNumber);
    CameraWidget *rootWidget, *widget;
    int ret;

    ret = gp_camera_get_config(camera->camera, &rootWidget, context);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get root widget")));
        return;
    }

    ret = gp_widget_get_child_by_name (rootWidget, configKey.toStdString().c_str(), &widget);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get widget for config key")).append(configKey));
        return;
    }


    ret = gp_widget_set_value(widget, value);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set camera value")));
        return;
    }

    ret = gp_camera_set_config(camera->camera, rootWidget, context);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set camera config")));
        return;
    }

    delete value;
}

void PMGPhotoCommandThread::liveViewError(QString message) {
    emit cameraError(message);
}

void PMGPhotoCommandThread::handlePreview(CameraFile *cameraFile) {
    emit previewAvailable(cameraFile);
}
