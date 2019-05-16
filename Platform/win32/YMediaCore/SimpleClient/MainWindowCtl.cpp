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

	//http://hc.yinyuetai.com/uploads/video
	main_win_->ctl_edit_url_->SetText(L"http://hc.yinyuetai.com/uploads/videos/common/B75F016ABA0C8B451DD41000A9425354.mp4?sc=e73ae1a1a7496ca5");
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
	if (w == 0 || h == 0 || !data)
		return;
	static EV_VIDEO_STRUCT *st = nullptr; 
	if (!st)
	{
		st = new EV_VIDEO_STRUCT;
		st->data = new char[2560*2560*4];
	}
	st->w = w;
	st->h = h;
	memcpy(st->data, data, w*h*4);

	PostMessage(main_win_->GetHWND(), EV_VIDEO, 0, (LPARAM)st);
}
void MainWindowCtl::SetDuration(int dur)
{
	PostMessage(main_win_->GetHWND(), EV_DUR, 0, (LPARAM)dur);
}

void MainWindowCtl::SetCurPos(int cur)
{
	PostMessage(main_win_->GetHWND(), EV_CUR, 0, (LPARAM)cur);
}

void MainWindowCtl::SetBufferPercent(float percent)
{
	PostMessage(main_win_->GetHWND(), EV_PERCENT, 0, (LPARAM)(percent*100));
}

void MainWindowCtl::SetError(int type)
{
	PostMessage(main_win_->GetHWND(), EV_ERROR, 0, (LPARAM)type);
}
