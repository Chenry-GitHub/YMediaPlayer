#pragma once
#include <QSlider>
#include <QPaintEvent>
class DerSlider:public QSlider
{
	Q_OBJECT
public:
	DerSlider(QWidget *parent =nullptr);
	~DerSlider();

	void SetBufferPercent(float percent);
protected:
	virtual void mousePressEvent(QMouseEvent *ev) override;
	
	virtual void paintEvent(QPaintEvent *e)override;
signals:
	void sig_Clicked(float value);

private:
	float percent_=0.0f;
};

