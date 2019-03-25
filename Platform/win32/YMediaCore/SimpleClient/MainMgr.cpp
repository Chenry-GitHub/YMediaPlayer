#include "stdafx.h"
#include "MainMgr.h"
#include "utils/qaqlog/qaqlog.h"



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

	player_ = CreatePlayer(MODE_WIN_WAV, MODE_USER, this);

	player_->SetDurationChangedFunction([](void*opa, int dur) {
		MainMgr * mgr = (MainMgr *)(opa);
		mgr->main_ctl_.SetDuration(dur);
	});
	player_->SetCurrentChangedFucnton([](void*opa, int cur) {
		MainMgr * mgr = (MainMgr *)(opa);
		mgr->main_ctl_.SetCurPos(cur);
	});

	player_->SetUserHandleVideoFunction([](void *opa, void*data, int width, int height) {
		MainMgr * mgr = (MainMgr *)(opa);
		mgr->main_ctl_.SetVideoImage(data, width, height);
	});

	player_->SetBufferFunction([](void *opa, float percent) {
		MainMgr * mgr = (MainMgr *)(opa);
		mgr->main_ctl_.SetBufferPercent(percent);
	});

}

void MainMgr::UnInit()
{
	main_ctl_.Close();
	DeletePlayer(player_);
}

void MainMgr::OnPlayClicked()
{
	std::string url = main_ctl_.GetPlayUrl();
	//MessageBoxA(NULL, url.c_str(), "cap", 0);
	printf("test");
	player_->SetMedia(url.c_str());
	player_->Play();
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
	player_->Seek(percent);
}
