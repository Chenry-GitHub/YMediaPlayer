#pragma once
#include <QSlider>
class DerSlider:public QSlider
{
	Q_OBJECT
public:
	DerSlider(QWidget *parent =nullptr);
	~DerSlider();
protected:
	virtual void mousePressEvent(QMouseEvent *ev) override;
	
signals:
	void sig_Clicked(float value);

};

