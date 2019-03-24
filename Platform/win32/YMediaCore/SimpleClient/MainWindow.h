#pragma once
#include "YBaseWindow.h"
#include "LabelVideo.h"
#include "DerMainSlider.h"


class MainWindow:public YBaseWindow
{
public:
	MainWindow();

	virtual void OnAfterCreate() override;

	LabelVideo * ctl_video_display_=nullptr;
	CControlUI * ctl_play_media_ = nullptr;
	CControlUI * ctl_edit_url_ = nullptr;
	CControlUI * ctl_progress_info_ = nullptr;
	DerMainSlider * ctl_progress_slider_ = nullptr;


	std::function<void(TNotifyUI& msg)> click_func_=nullptr;

	virtual CControlUI* CreateControl(LPCTSTR pstrClass) override;


	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

protected:
	virtual UILIB_RESOURCETYPE GetResourceType() const override;

	virtual LPCTSTR GetResourceID() const override;

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual void OnClick(TNotifyUI& msg) override;

protected:
	virtual CDuiString GetSkinFolder() override;

	virtual CDuiString GetSkinFile() override;

private:
	~MainWindow();

	int dur_=0;
};

