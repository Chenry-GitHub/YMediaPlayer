#include "stdafx.h"
#include "MainMgr.h"
#include "utils/qaqlog/qaqlog.h"
#include "YMediaComm.h"



MainMgr::MainMgr()
{
}


MainMgr::~MainMgr()
{
}

void MainMgr::Init()
{
#if _DEBUG
	lg::Initialize();
	lg::AddConsoleLogger(lg::info);
	lg::AddConsoleLogger(lg::debug);
#endif
	main_ctl_.CreateMainWindow();
	main_ctl_.btn_clicked_paly_media_func_ = std::bind(&MainMgr::OnPlayClicked,this);
	main_ctl_.SetCloseFunction(std::bind(&MainMgr::OnClickedClose,this));
	main_ctl_.SetFinalFunction(std::bind(&MainMgr::OnMainWindowFinalMSG, this));
	main_ctl_.SetClickedSliderFunction(std::bind(&MainMgr::OnSliderClicked, this, std::placeholders::_1));

	player_ = CreatePlayer(MODE_WIN_WAV, MODE_USER);
	player_->setDelegate(this);
	player_->setOpaque(this);
}

void MainMgr::UnInit()
{
	main_ctl_.Close();
	DeletePlayer(player_);
}

void MainMgr::onDurationChanged(YMediaPlayer* player, int duration)
{
	MainMgr * mgr = (MainMgr *)(player->getOpaque());
	mgr->main_ctl_.SetDuration(duration);
}

void MainMgr::onCurrentChanged(YMediaPlayer*player, int pos)
{
	MainMgr * mgr = (MainMgr *)(player->getOpaque());
	mgr->main_ctl_.SetCurPos(pos);
}

void MainMgr::onStatusChanged(YMediaPlayer*player, PlayerStatus status)
{
	MainMgr * mgr = (MainMgr *)(player->getOpaque());
	switch (status)
	{
	case PlayerStatus::Stop:
		break;
	case PlayerStatus::Pause:
		break;
	case PlayerStatus::Playing:
		break;
	case PlayerStatus::Buffering:
		break;
	case PlayerStatus::Done:
		break;
	case PlayerStatus::ErrorUrl:
	{
		mgr->main_ctl_.SetError(0);
	}
	break;
	case PlayerStatus::ErrorFormat:
	{
		mgr->main_ctl_.SetError(1);
	}
	break;
	case PlayerStatus::ErrorTimeOut:
	{
		mgr->main_ctl_.SetError(2);
	}
	break;
	case PlayerStatus::ErrorUserInterrupt:
	{
		mgr->main_ctl_.SetError(3);
	}
	break;
	default:
		break;
	}
}

void MainMgr::onVideoData(YMediaPlayer*player, void *data, int width, int height)
{
	MainMgr * mgr = (MainMgr *)(player->getOpaque());
	mgr->main_ctl_.SetVideoImage(data, width, height);
}

void MainMgr::onNetworkBuffer(YMediaPlayer*player, float percent)
{
	MainMgr * mgr = (MainMgr *)(player->getOpaque());
	mgr->main_ctl_.SetBufferPercent(percent);
}

void MainMgr::OnPlayClicked()
{
	std::string url = main_ctl_.GetPlayUrl();
	//MessageBoxA(NULL, url.c_str(), "cap", 0);
	printf("test");
	player_->setMedia(url.c_str());
	player_->play();
}

bool MainMgr::OnClickedClose()
{
	
	return true;
}

bool MainMgr::OnMainWindowFinalMSG()
{
	PostQuitMessage(0);
	return true;
}

void MainMgr::OnSliderClicked(float percent)
{
	player_->seek(percent);
}
