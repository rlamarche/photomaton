#ifndef PMGRAPHICSVIEW_H
#define PMGRAPHICSVIEW_H

#include <QGraphicsView>

#include <gphoto2/gphoto2-camera.h>

class PMGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit PMGraphicsView(QWidget *parent = 0);
    ~PMGraphicsView();
signals:
    
public slots:
    void displayPreview(CameraFile* cameraFile);
    void zoomIn();
    void zoomOut();

protected:
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event);
#endif

    void scaleView(qreal scaleFactor);
    
    QGraphicsPixmapItem *preview;
    QPixmap *pixmap;
};

#endif // PMGRAPHICSVIEW_H
