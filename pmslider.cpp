#include "pmslider.h"

#include <QKeyEvent>

PMSlider::PMSlider(QWidget *parent) :
    QSlider(parent)
{
}

void PMSlider::keyReleaseEvent(QKeyEvent *keyEvent) {
    /*switch (keyEvent->key()) {
    case Qt::Key_Left:
        setValue(value() - 1);
        break;
    case Qt::Key_Right:
        setValue(value() + 1);
        break;
    default: break;
    }*/
    emit keyReleased();
}
