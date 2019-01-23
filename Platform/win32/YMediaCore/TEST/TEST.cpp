#include "TEST.h"
#include "DerSlider.h"
#include "YMediaCore.h"

#include <QTime>
#include <QDebug>
#include <QFileDialog>

TEST::TEST(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	QObject::connect(this, &TEST::sig_Dur, this, [&](int dur) {
		QTime tim(dur /3600,dur/60, dur % 60,0);
		QString str = tim.toString("hh:mm:ss");
		 ui.lab_dur->setText(str);
		 ui.slider_media->setMaximum(dur);
		 ui.slider_media->setMinimum(0);
		 ui.slider_media->setValue(0);
	});

	QObject::connect(this, &TEST::sig_Pos, this, [&](int curpos) {
		QTime tim(curpos / 3600, curpos / 60, curpos % 60, 0);
		QString str = tim.toString("hh:mm:ss");
		ui.lab_cur->setText(str);
		ui.slider_media->setValue(curpos);
	});

	QObject::connect(ui.slider_media, &DerSlider::sig_Clicked, this, [&](float value)
	{
		player_->Seek(value);
	});

	QObject::connect(ui.btn_play, &QPushButton::clicked, this, [&] {
		if(player_->IsPlaying())
			player_->Pause();
		else
			player_->Play();
	});

	QObject::connect(ui.btn_open, &QPushButton::clicked, this, [&] {

		QString fileName = QFileDialog::getOpenFileName(nullptr,
			u8"文件对话框！",
			nullptr,
			"*.*");
		if (fileName.isEmpty())
			return;
		player_->SetMediaFromFile(fileName.toStdString().c_str());
		player_->Play();
	});

	std::function<void (int)> dur_func = std::bind(&TEST::sig_Dur, this, std::placeholders::_1);
	std::function <void(int)> cur_func = std::bind(&TEST::sig_Pos, this, std::placeholders::_1);

	
	player_ = new YMediaPlayer(MODE_WIN_WAV , MODE_WIN_GDI);
	player_->SetDisplayWindow((void*)ui.lab_video->winId());

//	player_->SetDurationChangedFunction(reinterpret_cast <DurFunc>(dur_func));
//	player_->SetCurrentChangedFucnton(reinterpret_cast <DurFunc>(0x22));
	player_->SetMediaFromFile("D:\\video3.mp4");
	player_->Play();

	player_->Pause();
}

TEST::~TEST()
{
	delete player_;
}
