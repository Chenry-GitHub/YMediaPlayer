#pragma once
#include "MainWindowCtl.h"
#include "YMediaPlayer.h"
class MainMgr:public YMediaPlayer::Delegate
{
public:
	MainMgr();
	~MainMgr();

	void Init();

	void UnInit();

protected:
	virtual void onDurationChanged(YMediaPlayer*, int duration) override;

	virtual void onCurrentChanged(YMediaPlayer*, int cur_pos) override;

	virtual void onStatusChanged(YMediaPlayer*, PlayerStatus status) override;

	virtual void onVideoData(YMediaPlayer*, void *data, int width, int height) override;

	virtual void onNetworkBuffer(YMediaPlayer*, float percent) override;

protected:
	void OnPlayClicked();
	
	bool OnClickedClose();

	bool OnMainWindowFinalMSG();

	void OnSliderClicked(float );

	MainWindowCtl main_ctl_;

	YMediaPlayer *player_;
};

