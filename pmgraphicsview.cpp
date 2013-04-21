#include "pmgraphicsview.h"

#include <QKeyEvent>
#include <QGraphicsPixmapItem>
#include <QTime>

#include <math.h>

PMGraphicsView::PMGraphicsView(QWidget *parent) :
    QGraphicsView(parent)
{
    QGraphicsScene *scene = new QGraphicsScene();

    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
   // scene->setSceneRect(0, 0, width(), height());
    //scene->setSceneRect(-200, -200, 400, 400);
    setScene(scene);
    //setCacheMode(CacheBackground);
    //setViewportUpdateMode(BoundingRectViewportUpdate);
    //setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    //scale(qreal(0.8), qreal(0.8));
    //setMinimumSize(400, 400);
    //setWindowTitle(tr("Elastic Nodes"));

    preview = new QGraphicsPixmapItem();
    //preview->setX(0);
    //preview->setY(0);
    scene->addItem(preview);

    pixmap = new QPixmap();

    preview->setPixmap(*pixmap);

    preview->setX(preview->boundingRect().width() / 1.0);
    preview->setY(preview->boundingRect().height() / 1.0);
}

PMGraphicsView::~PMGraphicsView() {
    delete pixmap;
}

void PMGraphicsView::resizeEvent(QResizeEvent *event) {
}

void PMGraphicsView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        translate(0, -20);
        break;
    case Qt::Key_Down:
        translate(0, 20);
        break;
    case Qt::Key_Left:
        translate(-20, 0);
        break;
    case Qt::Key_Right:
        translate(20, 0);
        break;
    case Qt::Key_Plus:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_Space:
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void PMGraphicsView::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}

void PMGraphicsView::scaleView(qreal scaleFactor)
{
    qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}

void PMGraphicsView::zoomIn()
{
    scaleView(qreal(1.2));
}

void PMGraphicsView::zoomOut()
{
    scaleView(1 / qreal(1.2));
}

void PMGraphicsView::displayPreview(CameraFile *cameraFile) {
    static QTime time;
    static int frameCount = 0;
    static QGraphicsTextItem fpsText;

    if (frameCount == 0) {
        time.start();
        scene()->addItem(&fpsText);
    }

    frameCount ++;

    unsigned long int size;
    const char *data;

    gp_file_get_data_and_size(cameraFile, &data, &size);

    pixmap->loadFromData((uchar*) data, size, "JPG");
    preview->setPixmap(*pixmap);

    gp_file_free(cameraFile);

    float fps = time.elapsed() / float(frameCount);
    fpsText.setPlainText(QString("FPS : %1").arg(fps));

}
