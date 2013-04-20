#ifndef PMSLIDER_H
#define PMSLIDER_H

#include <QSlider>

class PMSlider : public QSlider
{
    Q_OBJECT
public:
    explicit PMSlider(QWidget *parent = 0);
protected:
    void keyReleaseEvent(QKeyEvent *);
signals:
    void keyReleased();
public slots:
    
};

#endif // PMSLIDER_H
