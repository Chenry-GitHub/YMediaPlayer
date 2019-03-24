#pragma once
#include "MainWindowCtl.h"
#include "YMediaPlayer.h"
class MainMgr
{
public:
	MainMgr();
	~MainMgr();

	void Init();

	void UnInit();
protected:
	void OnPlayClicked();
	
	bool OnClickedClose();

	bool OnMainWindowFinalMSG();

	void OnSliderClicked(float );

	MainWindowCtl main_ctl_;

	YMediaPlayer *player_;
};

