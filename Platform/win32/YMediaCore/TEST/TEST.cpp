#include "TEST.h"
#include "DerSlider.h"
#include "AboutUs.h"

#include "YMediaCore.h"

#include <QTime>
#include <QDebug>
#include <QFileDialog>


TEST *g_global_;

TEST::TEST(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	g_global_ = this;
	ui.le_media_url->setPlaceholderText("Please Input URL Here~");


	QObject::connect(this, &TEST::sig_Dur, this, [&](int dur) {
		QTime tim(dur / 3600, dur / 60, dur % 60, 0);
		QString str = tim.toString("hh:mm:ss");
		ui.lab_dur->setText(str);
		ui.slider_media->setMaximum(dur);
		ui.slider_media->setMinimum(0);
		ui.slider_media->setValue(0);
	},Qt::QueuedConnection);


	QObject::connect(this, &TEST::sig_Pos, this, [&](int curpos) {
		QTime tim(curpos / 3600, curpos / 60, curpos % 60, 0);
		QString str = tim.toString("hh:mm:ss");
		ui.lab_cur->setText(str);
		ui.slider_media->setValue(curpos);
	}, Qt::QueuedConnection);

	QObject::connect(ui.slider_media, &DerSlider::sig_Clicked, this, [&](float value)
	{
		player_->Seek(value);
	});

	QObject::connect(ui.btn_play, &QPushButton::clicked, this, [&] {
		if (player_->IsPlaying())
			player_->Pause();
		else
			player_->Play();
	});

	QObject::connect(ui.btn_support, &QPushButton::clicked, this, [&] {
		AboutUs us(this);
		us.exec();
	});


	QObject::connect(ui.btn_open, &QPushButton::clicked, this, [&] {

		QString &&fileName = ui.le_media_url->text();
		if (fileName.isEmpty())
			return;
		player_->SetMediaFromFile(fileName.toStdString().c_str());
		player_->Play();
	});

	player_ = new YMediaPlayer(MODE_WIN_WAV, MODE_WIN_GDI);
	player_->SetDisplayWindow((void*)ui.lab_video->winId());

	player_->SetDurationChangedFunction([](int dur) {
		emit g_global_->sig_Dur(dur);
	});
	player_->SetCurrentChangedFucnton([](int cur){
		emit g_global_->sig_Pos(cur);
	}); 
}

TEST::~TEST()
{
	delete player_;
}
