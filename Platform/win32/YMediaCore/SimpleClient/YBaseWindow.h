#pragma once

enum U_WIN_MSG{
	U_MSG_WIN_CLOSE =  WM_USER+5,

};


class YBaseWindow:public WindowImplBase
{
public:
	YBaseWindow();
	~YBaseWindow();

	std::function <bool()> close_func_;

	std::function <void ()> final_message_func_;

	void ShowWindow(bool bShow /* = true */, bool bTakeFocus /* = true */);

	virtual LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled) override;

	virtual LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

	virtual LRESULT OnNcActivate(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) override
	{
		return true;
	}

	virtual void OnAfterCreate() {}

	virtual LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) override;

	virtual void OnFinalMessage(HWND hWnd) override;
	
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) override;

protected:
	virtual LPCTSTR GetWindowClassName() const override;

	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;



	//virtual CDuiString GetSkinFolder() override;


	//virtual CDuiString GetSkinFile() override;

	virtual CDuiString GetSkinFolder() override;


	virtual CDuiString GetSkinFile() override;

	

};

