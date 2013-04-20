#include "pmgphotoliveviewgphotothread.h"

PMGPhotoLiveViewGPhotoThread::PMGPhotoLiveViewGPhotoThread(GPContext *context, PMCamera *camera) :
    QThread(0), context(context), camera(camera)

{
    stop = false;
}

void PMGPhotoLiveViewGPhotoThread::stopNow() {
    stop = true;
    wait();
}

void PMGPhotoLiveViewGPhotoThread::restart() {
    stop = false;
    start();
}

void PMGPhotoLiveViewGPhotoThread::run() {
    forever {
        CameraFile *file;
        //unsigned long int size;
        //const char *data;

        int ret = gp_file_new(&file);
        if (ret < GP_OK) {
            emit cameraError(tr("Unable to create camera file"), ret);
            break;
        }

        ret = gp_camera_capture_preview(camera->camera, file, context);
        if (ret < GP_OK) {
            emit cameraError(tr("Unable to capture preview"), ret);
            gp_file_free(file);
            break;
        }
        if (stop) {
            gp_file_free(file);
            break;
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

    emit liveViewStopped(camera->cameraNumber);
}
