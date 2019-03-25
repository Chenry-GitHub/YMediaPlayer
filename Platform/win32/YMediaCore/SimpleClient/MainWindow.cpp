#include "stdafx.h"
#include "MainWindow.h"


MainWindow::MainWindow()
{
	
}


MainWindow::~MainWindow()
{
}


void MainWindow::OnAfterCreate()
{
	ctl_video_display_ = (LabelVideo *)m_PaintManager.FindControl(L"display");
	ctl_edit_url_ = m_PaintManager.FindControl(L"edit_url");
	ctl_play_media_ = m_PaintManager.FindControl(L"btn_play");
	ctl_progress_info_ = m_PaintManager.FindControl(L"progress_info");
	ctl_progress_slider_ = (DerMainSlider*)m_PaintManager.FindControl(L"progress_slider");

	
}




DuiLib::CControlUI* MainWindow::CreateControl(LPCTSTR pstrClass)
{
	if (_tcsicmp(pstrClass, LabelVideo::classname_.GetData()) == 0)
	{
		return new LabelVideo;
	}
	else if (_tcsicmp(pstrClass, DerMainSlider::classname_.GetData()) == 0)
	{
		return new DerMainSlider;
	}
	return __super::CreateControl(pstrClass);
}

LRESULT MainWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (uMsg == EV_VIDEO)
	{
		EV_VIDEO_STRUCT *st = (EV_VIDEO_STRUCT *)lParam;
		ctl_video_display_->SetImage(st->data, st->w, st->h);
	}
	else if (uMsg == EV_CUR)
	{
		int value = (int)lParam;
		ctl_progress_slider_->SetCurValue(value);
	
		CDuiString str;
		str.Format(L"%02d:%02d:%02d/%2d:%02d:%02d", value / 3600, value / 60, value % 60, dur_/ 3600, dur_ / 60, dur_ % 60 );
		ctl_progress_info_->SetText(str);
	}
	else if (uMsg == EV_DUR)
	{
		int value = (int)lParam;
		dur_ = value;
		ctl_progress_slider_->SetRange(0,value);

		CDuiString str;
		str.Format(L"%02d:%02d:%02d/%2d:%02d:%02d", 0,0,0, dur_ / 3600, dur_ / 60, dur_ % 60 );
		ctl_progress_info_->SetText(str);
	}
	else if (uMsg == EV_PERCENT)
	{
		float value = (float)lParam/100;
			
		ctl_progress_slider_->SetBufferPercent(value);
	}
	return __super::HandleCustomMessage(uMsg, wParam, lParam,bHandled);
}

DuiLib::UILIB_RESOURCETYPE MainWindow::GetResourceType() const
{
	return UILIB_ZIPRESOURCE;
}

LPCTSTR MainWindow::GetResourceID() const
{
	return MAKEINTRESOURCE(IDR_ZIP_MAINWINDOW);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return __super::HandleMessage(uMsg, wParam, lParam);
}

void MainWindow::OnClick(TNotifyUI& msg)
{
		if (click_func_)
			click_func_(msg);
		return __super::OnClick(msg);
}

DuiLib::CDuiString MainWindow::GetSkinFolder()
{
	return L"";
}

DuiLib::CDuiString MainWindow::GetSkinFile()
{
	return L"MainWindow.xml";
}
