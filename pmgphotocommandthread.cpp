#include "pmgphotocommandthread.h"

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

PMLiveViewGPhotoThread::PMLiveViewGPhotoThread(GPContext *context, PMCamera *camera) :
    QThread(0), context(context), camera(camera)

{
    stop = false;
}

void PMLiveViewGPhotoThread::stopNow() {
    stop = true;
    wait();
}

void PMLiveViewGPhotoThread::run() {
    forever {
        if (stop) {
            break;
        }
        CameraFile *file;
        //unsigned long int size;
        //const char *data;

        int ret = gp_file_new(&file);
        if (ret < GP_OK) {
            emit cameraError(tr("Erreur de connexion à la caméra", "Impossible d'obtenir une image preview."));
            return;
        }

        ret = gp_camera_capture_preview(camera->camera, file, context);
        if (ret < GP_OK) {
            emit cameraError(tr("Erreur de connexion à la caméra", "Impossible de capturer une image"));
            return;
        }

        emit previewAvailable(file);
    /*
        printf("%d", size);

        QFile qfile("/tmp/out.jpg");
        qfile.open(QIODevice::WriteOnly);

        qfile.write(data, size);
        qfile.close();
    */
        /*if (!pixmap.loadFromData((uchar*) data, size, "JPG")) {
            QMessageBox::critical(this, "Erreur de connexion à la caméra", "Impossible de lire les données renvoyées par la caméra.");
            return;
        }*/

        //this->ui->widget->update();

    }

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

            gp_camera_unref(camera->camera);
            delete camera;
        }

        delete cameras;
    }

    QList<PMLiveViewGPhotoThread*>::iterator t;
    for (t = liveviewThreads.begin(); t != liveviewThreads.end(); ++t) {
        PMLiveViewGPhotoThread *thread = (*t);
        thread->stopNow();
        delete thread;
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
                break;
                case START_LIVEVIEW:
                    commandStartLiveView(*((int*) command.args));
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
        camera->liveview = false;

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

    if (!camera->liveview) {
        PMLiveViewGPhotoThread *liveviewthread = new PMLiveViewGPhotoThread(context, cameras->at(cameraNumber));

        connect(liveviewthread, SIGNAL(cameraError(QString)), this, SLOT(liveViewError(QString)));
        connect(liveviewthread, SIGNAL(previewAvailable(CameraFile*)), this, SLOT(handlePreview(CameraFile*)));

        liveviewThreads.append(liveviewthread);

        liveviewthread->start();

        camera->liveview = true;
    }
}

void PMGPhotoCommandThread::liveViewError(QString message) {
    emit cameraError(message);
}

void PMGPhotoCommandThread::handlePreview(CameraFile *cameraFile) {
    emit previewAvailable(cameraFile);
}
