#include "pmgphotocommandthread.h"
#include "pmgphotoliveviewgphotothread.h"
#include "pmgphototetheredthread.h"

#include <QMessageBox>
#include <QTime>

#include <iostream>

PMGPhotoCommandThread::PMGPhotoCommandThread(QObject *parent) :
    QThread(parent)
{
    abort = false;
    cameras = 0;
    context = 0;
    abilitieslist = 0;
    portinfolist = 0;

    connect(this, SIGNAL(cameraError(QString,int)), this, SLOT(handleError(QString,int)));
}

void PMGPhotoCommandThread::stopNow() {
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
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
            if (camera->tetheredThread) {
                camera->tetheredThread->stopNow();
                delete camera->tetheredThread;
            }
            if (camera->window) {
                gp_widget_free(camera->window);
            }
            if (camera->camera) {
                gp_camera_unref(camera->camera);
            }
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
        bool liveview = false;
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

        if (command.cameraNumber > -1) {
            PMCamera* camera = cameras->at(command.cameraNumber);
            if (camera->liveviewThread && camera->liveviewThread->isRunning() && command.type != START_LIVEVIEW && command.type != STOP_LIVEVIEW) {
                liveview = true;
                camera->liveviewThread->stopNow();
            }
        }

        if (!empty) {
            switch (command.type) {
            case AUTODETECT_CAMERAS:
                commandAutodetect();
                break;
            case OPEN_CAMERA:
                commandOpenCamera(command.cameraNumber);
                break;
            case START_LIVEVIEW:
                commandStartLiveView(command.cameraNumber);
                break;
            case STOP_LIVEVIEW:
                commandStopLiveView(command.cameraNumber);
                break;
            case SET_WIDGET_VALUE:
                commandSetWidgetValue(command.cameraNumber, command.widgetValue);
                delete command.widgetValue;
                break;
            case UPDATE_WIDGETS:
                commandUpdateWidgets(command.cameraNumber);
                break;
            }
        }

        if (command.cameraNumber > -1) {
            PMCamera* camera = cameras->at(command.cameraNumber);
            if (liveview) {
                camera->liveviewThread->restart();
            }

        }

        mutex.lock();
        if (commandQueue.isEmpty()) {
            condition.wait(&mutex);
        }
        mutex.unlock();
    }
}

void PMGPhotoCommandThread::autodetect() {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = AUTODETECT_CAMERAS;
    command.cameraNumber = -1;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::openCamera(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = OPEN_CAMERA;
    command.cameraNumber = cameraNumber;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::startLiveView(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = START_LIVEVIEW;
    command.cameraNumber = cameraNumber;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::stopLiveView(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;
    command.type = STOP_LIVEVIEW;
    command.cameraNumber = cameraNumber;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::setWidgetValue(int cameraNumber, PMCommand_SetWidgetValue *value) {
    QMutexLocker locker(&mutex);

    PMCommand command;

    command.type = SET_WIDGET_VALUE;
    command.cameraNumber = cameraNumber;
    command.widgetValue = value;

    commandQueue.append(command);
    condition.wakeOne();
}

void PMGPhotoCommandThread::updateWidgets(int cameraNumber) {
    QMutexLocker locker(&mutex);

    PMCommand command;

    command.type = UPDATE_WIDGETS;
    command.cameraNumber = cameraNumber;

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
        camera->tetheredThread = 0;
        camera->camera = 0;
        camera->window = 0;

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
        emit cameraError(QString(tr("Unable to create camera")), ret);
        return;
    }

    /* First lookup the model / driver */
    int m = gp_abilities_list_lookup_model (abilitieslist, camera->model.toStdString().c_str());
    if (m < GP_OK) {
        emit cameraError(QString(tr("Unable to lookup model")), ret);
        return;
    }
    ret = gp_abilities_list_get_abilities (abilitieslist, m, &a);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get abilities list")), ret);
        return;
    }
    ret = gp_camera_set_abilities (camera->camera, a);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set abilities")), ret);
        return;
    }

    /* Then associate the camera with the specified port */
    int p = gp_port_info_list_lookup_path (portinfolist, camera->port.toStdString().c_str());

    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to lookup port")), ret);
        return;
    }
    ret = gp_port_info_list_get_info (portinfolist, p, &pi);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get info on port")), ret);
        return;
    }
    ret = gp_camera_set_port_info (camera->camera, pi);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set port info on camera")), ret);
        return;
    }

    // Look for widgets
    emit startListingWidgets(camera->cameraNumber);
    ret = gp_camera_get_config(camera->camera, &camera->window, context);
    if (ret < GP_OK) {
        emit cameraError(tr("Unable to get root widget"), ret);
        return;
    }

    ret = findWidgets(camera->cameraNumber, camera->window);
    if (ret < GP_OK) {
        emit cameraError(tr("Unable to find widgets"), ret);
        return;
    }



    // Start tethered thread
    camera->tetheredThread = new PMGPhotoTetheredThread(context, camera);

    connect(camera->tetheredThread, SIGNAL(cameraError(QString, int)), this, SLOT(handleError(QString, int)));
    connect(camera->tetheredThread, SIGNAL(cameraStatus(QString)), this, SLOT(handleCameraStatus(QString)));

   // camera->tetheredThread->start();

    emit cameraOpened(camera->cameraNumber);
    emit cameraStatus(QString(tr("Camera successfuly open")));

}

void PMGPhotoCommandThread::commandUpdateWidgets(int cameraNumber) {
    PMCamera* camera = cameras->at(cameraNumber);

    if (camera->window) {
        gp_widget_free(camera->window);
    }

    int ret = gp_camera_get_config(camera->camera, &camera->window, context);
    if (ret < GP_OK) {
        emit cameraError(tr("Unable to get root widget"), ret);
        return;
    }

    ret = findWidgets(camera->cameraNumber, camera->window);
    if (ret < GP_OK) {
        emit cameraError(tr("Unable to find widgets"), ret);
        return;
    }
}

int PMGPhotoCommandThread::findWidgets(int cameraNumber, CameraWidget *widget) {
    int ret;
    int n = gp_widget_count_children(widget);
    CameraWidget* child;

    for (int i = 0; i < n; i ++) {
        int ret = gp_widget_get_child(widget, i, &child);
        if (ret < GP_OK) {
            return ret;
        }

        emit newWidget(cameraNumber, child);

        ret = findWidgets(cameraNumber, child);
        if (ret < GP_OK) {
            return ret;
        }
    }

    return GP_OK;
}

void PMGPhotoCommandThread::commandStartLiveView(int cameraNumber) {
    PMCamera *camera = cameras->at(cameraNumber);

    if (!camera->liveviewThread) {
        PMGPhotoLiveViewGPhotoThread *liveviewthread = new PMGPhotoLiveViewGPhotoThread(context, cameras->at(cameraNumber));
        camera->liveviewThread = liveviewthread;

        connect(liveviewthread, SIGNAL(cameraError(QString, int)), this, SLOT(handleError(QString, int)));
        connect(liveviewthread, SIGNAL(previewAvailable(CameraFile*)), this, SLOT(handlePreview(CameraFile*)));
    }
    if (!camera->liveviewThread->isRunning()) {
        camera->liveviewThread->start();
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

void PMGPhotoCommandThread::commandSetWidgetValue(int cameraNumber, PMCommand_SetWidgetValue* value) {
    PMCamera *camera = cameras->at(cameraNumber);
    CameraWidget *widget;
    QString strValue;
    QString radioValue;
    int ret;

    /*
    ret = gp_camera_get_config(camera->camera, &rootWidget, context);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get root widget")), ret);
        return;
    }*/

    ret = gp_widget_get_child_by_name (camera->window, value->configKey.toStdString().c_str(), &widget);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to get widget for config key %1").arg(value->configKey)), ret);
        return;
    }

    ret = GP_OK;

    switch (value->valueType) {
    case PMCommand_SetWidgetValue::TOGGLE_VALUE:
        ret = gp_widget_set_value(widget, &value->toggleValue);
        strValue.sprintf("%d", value->toggleValue);
        break;

    case PMCommand_SetWidgetValue::RADIO_VALUE:
        radioValue = QString(*value->radioValue);
        delete value->radioValue;
        ret = gp_widget_set_value(widget, radioValue.toStdString().c_str());
        strValue = radioValue;
        break;

    case PMCommand_SetWidgetValue::RANGE_VALUE:
        ret = gp_widget_set_value(widget, &value->rangeValue);
        strValue.sprintf("%f", value->rangeValue);
        break;
    }

    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set camera value")), ret);
        return;
    }

    ret = gp_camera_set_config(camera->camera, camera->window, context);
    if (ret < GP_OK) {
        emit cameraError(QString(tr("Unable to set camera config")), ret);
        return;
    }

    emit cameraStatus(tr("Set value %1 to %2 successful").arg(value->configKey, strValue));

    //updateWidgets(cameraNumber);
}

void PMGPhotoCommandThread::handleError(const QString& message, int errorCode) {
    QString msg = pm_gp_port_result_as_string(errorCode);
    QString errorMessage(message);
    emit cameraErrorString(errorMessage.append(" (").append(msg).append(")"));
}

void PMGPhotoCommandThread::handleCameraStatus(const QString &message) {
    emit cameraStatus(message);
}

void PMGPhotoCommandThread::handlePreview(CameraFile *cameraFile) {
    emit previewAvailable(cameraFile);
}

void PMGPhotoCommandThread::handleLiveViewStopped(int cameraNumber) {
    emit liveViewStopped(cameraNumber);
}

QString
pm_gp_port_result_as_string (int result)
{
    switch (result) {
    case GP_OK:
        return QObject::tr("No error");
    case GP_ERROR:
        return QObject::tr("Unspecified error");
    case GP_ERROR_IO:
        return QObject::tr("I/O problem");
    case GP_ERROR_BAD_PARAMETERS:
        return QObject::tr("Bad parameters");
    case GP_ERROR_NOT_SUPPORTED:
        return QObject::tr("Unsupported operation");
    case  GP_ERROR_FIXED_LIMIT_EXCEEDED:
        return QObject::tr("Fixed limit exceeded");
    case GP_ERROR_TIMEOUT:
        return QObject::tr("Timeout reading from or writing to the port");
    case GP_ERROR_IO_SUPPORTED_SERIAL:
        return QObject::tr("Serial port not supported");
    case GP_ERROR_IO_SUPPORTED_USB:
        return QObject::tr("USB port not supported");
    case GP_ERROR_UNKNOWN_PORT:
        return QObject::tr("Unknown port");
    case GP_ERROR_NO_MEMORY:
        return QObject::tr("Out of memory");
    case GP_ERROR_LIBRARY:
        return QObject::tr("Error loading a library");
    case GP_ERROR_IO_INIT:
        return QObject::tr("Error initializing the port");
    case GP_ERROR_IO_READ:
        return QObject::tr("Error reading from the port");
    case GP_ERROR_IO_WRITE:
        return QObject::tr("Error writing to the port");
    case GP_ERROR_IO_UPDATE:
        return QObject::tr("Error updating the port settings");
    case GP_ERROR_IO_SERIAL_SPEED:
        return QObject::tr("Error setting the serial port speed");
    case GP_ERROR_IO_USB_CLEAR_HALT:
        return QObject::tr("Error clearing a halt condition on the USB port");
    case GP_ERROR_IO_USB_FIND:
        return QObject::tr("Could not find the requested device on the USB port");
    case GP_ERROR_IO_USB_CLAIM:
        return QObject::tr("Could not claim the USB device");
    case GP_ERROR_IO_LOCK:
        return QObject::tr("Could not lock the device");
    case GP_ERROR_HAL:
        return QObject::tr("libhal error");
    case GP_ERROR_CORRUPTED_DATA:
        return QObject::tr("Corrupted data received");
    case GP_ERROR_FILE_EXISTS:
        return QObject::tr("File already exists");
    case GP_ERROR_MODEL_NOT_FOUND:
        return QObject::tr("Specified camera model was not found");
    case GP_ERROR_DIRECTORY_NOT_FOUND:
        return QObject::tr("Specified directory was not found")        ;
    case GP_ERROR_FILE_NOT_FOUND:
        return QObject::tr("Specified directory was not found");
    case GP_ERROR_DIRECTORY_EXISTS:
        return QObject::tr("Specified directory already exists");
    case GP_ERROR_CAMERA_BUSY:
        return QObject::tr("The camera is already busy");
    case GP_ERROR_PATH_NOT_ABSOLUTE:
        return QObject::tr("Path is not absolute");
    case GP_ERROR_CANCEL:
        return QObject::tr("Cancellation successful");
    case GP_ERROR_CAMERA_ERROR:
        return QObject::tr("Unspecified camera error");
    case GP_ERROR_OS_FAILURE:
        return QObject::tr("Unspecified failure of the operating system");
    case GP_ERROR_NO_SPACE:
        return QObject::tr("Not enough space");
    default:
        return QObject::tr("Unknown error %1").arg(QString().sprintf("%d", result));
    }
}

