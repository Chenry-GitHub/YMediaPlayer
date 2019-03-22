#include "DerSlider.h"

#include <QMouseEvent>
#include <QPainter>

DerSlider::DerSlider(QWidget *parent)
	:QSlider(parent)
{
}


DerSlider::~DerSlider()
{
}

void DerSlider::SetBufferPercent(float percent)
{
	percent_ = percent;
	update(rect());
}

void DerSlider::mousePressEvent(QMouseEvent *ev)
{
	emit sig_Clicked((float)ev->pos().x()/this->width());
}

void DerSlider::paintEvent(QPaintEvent *e)
{
	__super::paintEvent(e);
	QPainter p(this);
	
	p.fillRect(0, 0, this->width()*percent_, this->height(), Qt::red);
	
		
}
