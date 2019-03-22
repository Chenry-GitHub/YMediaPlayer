#pragma once

#include <QtWidgets/QMainWindow>
#include <QImage>
#include <QPaintEvent>
#include "ui_TEST.h"

class YMediaPlayer;
class TEST : public QMainWindow
{
	Q_OBJECT

public:
	TEST(QWidget *parent = Q_NULLPTR);
	~TEST();

protected:
	virtual void paintEvent(QPaintEvent *event);
signals:
	void sig_Dur(int);

	void sig_Pos(int);

	void sig_Update(void *,int ,int);

	void sig_Buffer(float );
private:
	Ui::TESTClass ui;

	YMediaPlayer * player_;
	QImage img_;
};
