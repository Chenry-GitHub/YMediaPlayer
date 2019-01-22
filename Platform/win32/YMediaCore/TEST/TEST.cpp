#include "TEST.h"
#include "DerSlider.h"
#include "YMediaCore.h"

#include <QTime>
#include <QDebug>

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
	},Qt::BlockingQueuedConnection);

	QObject::connect(this, &TEST::sig_Pos, this, [&](int curpos) {
		QTime tim(curpos / 3600, curpos / 60, curpos % 60, 0);
		QString str = tim.toString("hh:mm:ss");
		ui.lab_cur->setText(str);
		ui.slider_media->setValue(curpos);
	}, Qt::BlockingQueuedConnection);

	QObject::connect(ui.slider_media, &DerSlider::sig_Clicked, this, [&](float value)
	{
		player_->Seek(value);
	});

	player_ = new YMediaPlayer(MODE_WIN_WAV , MODE_WIN_GDI);
	player_->SetDisplayWindow((void*)ui.lab_video->winId());
	player_->SetDurationChangedFunction(std::bind(&TEST::sig_Dur,this,std::placeholders::_1));
	player_->SetCurrentChangedFucnton(std::bind(&TEST::sig_Pos, this, std::placeholders::_1));
	player_->SetMediaFromFile("D:\\video3.mp4");
	player_->Play();

}
