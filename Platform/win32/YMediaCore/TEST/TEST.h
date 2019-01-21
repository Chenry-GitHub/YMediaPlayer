#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_TEST.h"

class YMediaPlayer;
class TEST : public QMainWindow
{
	Q_OBJECT

public:
	TEST(QWidget *parent = Q_NULLPTR);
signals:
	void sig_Dur(int);

	void sig_Pos(int);
private:
	Ui::TESTClass ui;

	YMediaPlayer * player_;
};
