#include "DerSlider.h"

#include <QMouseEvent>

DerSlider::DerSlider(QWidget *parent)
	:QSlider(parent)
{
}


DerSlider::~DerSlider()
{
}

void DerSlider::mousePressEvent(QMouseEvent *ev)
{
	emit sig_Clicked((float)ev->pos().x()/this->width());
}
