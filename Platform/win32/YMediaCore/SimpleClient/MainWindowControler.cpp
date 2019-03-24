#include "stdafx.h"
#include "MainWindowControler.h"

#include "MainWindow.h"

MainWindowControler::MainWindowControler()
	:cur_size_(0)
	, main_win_(nullptr)
{
}


MainWindowControler::~MainWindowControler()
{

}

HWND MainWindowControler::GetWindowWnd()
{
	return main_win_->GetHWND();
}

void MainWindowControler::CreateMainWindow()
{
	main_win_ = new MainWindow();
	main_win_->Create(NULL, _T("QAQUpdate"), UI_WNDSTYLE_FRAME, WS_EX_STATICEDGE, 0, 0, 484,334);
	main_win_->CenterWindow();
}

void MainWindowControler::CloseMainWindow()
{
	main_win_->Close();
}


void MainWindowControler::ShowMainWindow(bool show)
{
	main_win_->ShowWindow(show,true);
}

void MainWindowControler::SetCloseFunction(std::function <bool()> close_func_)
{
	main_win_->close_func_ = close_func_;
}

void MainWindowControler::SetFinalFunction(std::function <void()> destory_func)
{
	main_win_->final_message_func_ = destory_func;
}

void MainWindowControler::SetUpdateText(const std::wstring &update_text)
{
	main_win_->SetUpdateText(update_text);
}

void MainWindowControler::SetDownloadPercent(int percent)
{
	wchar_t buff[128];
	wsprintf(buff, L"%d", percent);
	main_win_->ctl_progress_download_->SetAttribute(L"value", buff);

	StrCatW(buff, L"%");
	main_win_->ctl_progress_text_->SetText(buff);
}

void MainWindowControler::SetDownloadCurSize(int size)
{
	cur_size_ = size;
	if (size < 1024)//B
	{
		wchar_t buff[128];
		wsprintf(buff, L"%0.2fB", (float)size);
		main_win_->ctl_cur_size_->SetText(buff);
	}
	else if(size <1048576)
	{
		wchar_t buff[128];
		wsprintf(buff, L"%0.2fKB", (float)size/1024);
		main_win_->ctl_cur_size_->SetText(buff);
	}
	else 
	{
		wchar_t buff[128];
		wsprintf(buff, L"%0.2fMB", (float)size / 1048576);
		main_win_->ctl_cur_size_->SetText(buff);
	}
	
}

void MainWindowControler::SetDownloadTotalSize(int size)
{
	total_size_ = size;
	if (size < 1024)//B
	{
		wchar_t buff[128];
		wsprintf(buff, L"¹²%0.2fB", (float)size);
		main_win_->ctl_total_size_->SetText(buff);
	}
	else  if (size <1048576)//KB
	{
		wchar_t buff[128];
		wsprintf(buff, L"¹²%0.2ffKB", (float)size/1024);
		main_win_->ctl_total_size_->SetText(buff);
	}
	else//MB
	{
		wchar_t buff[128];
		wsprintf(buff, L"¹²%0.2fMB", (float)size / 1048576);
		main_win_->ctl_total_size_->SetText(buff);
	}
}

