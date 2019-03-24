#include "stdafx.h"
#include "MainWindowCtl.h"
#include "MainWindow.h"
#include "utils\utils_str.h"

MainWindowCtl::MainWindowCtl()
{
	main_win_ = new MainWindow();

	main_win_->click_func_ = std::bind([&](TNotifyUI& msg) {
	
		if(msg.pSender == main_win_->ctl_play_media_)
		{
			if (btn_clicked_paly_media_func_)
				btn_clicked_paly_media_func_();
		}
		//else if ()
		{

		}
	
	},std::placeholders::_1);
}


MainWindowCtl::~MainWindowCtl()
{
}

void MainWindowCtl::SetCloseFunction(std::function<bool()> func)
{
	main_win_->close_func_ = func;
}

void MainWindowCtl::SetFinalFunction(std::function <void()> destory_func)
{
	main_win_->final_message_func_ = destory_func;
}


void MainWindowCtl::SetClickedSliderFunction(std::function <void(float)> func)
{
	main_win_->ctl_progress_slider_->clicked_func_ = func;
}

void MainWindowCtl::CreateMainWindow()
{
	main_win_->Create(NULL, _T("YMainWindow"), UI_WNDSTYLE_FRAME, WS_EX_STATICEDGE, 0, 0, 484, 334);
	main_win_->CenterWindow();

	main_win_->ctl_edit_url_->SetText(L"http://hc.yinyuetai.com/uploads/videos/common/02D30168547A579F07E92D27B0DA34D0.mp4?sc=068dd881b47be705");
}

void MainWindowCtl::Close()
{
	main_win_->Close();
}

std::string MainWindowCtl::GetPlayUrl()
{
	return utils::str::ws2s(main_win_->ctl_edit_url_->GetText().GetData());
}

void MainWindowCtl::SetVideoImage(void* data, int w, int h)
{
	EV_VIDEO_STRUCT st;
	st.data = data;
	st.w = w;
	st.h = h;

	SendMessage(main_win_->GetHWND(), EV_VIDEO, 0, (LPARAM)&st);
	//main_win_->ctl_video_display_->SetImage(data, w, h);
}
void MainWindowCtl::SetDuration(int dur)
{
	SendMessage(main_win_->GetHWND(), EV_DUR, 0, (LPARAM)&dur);
}

void MainWindowCtl::SetCurPos(int cur)
{
	SendMessage(main_win_->GetHWND(), EV_CUR, 0, (LPARAM)&cur);
}

void MainWindowCtl::SetBufferPercent(float percent)
{
	SendMessage(main_win_->GetHWND(), EV_PERCENT, 0, (LPARAM)&percent);
}
