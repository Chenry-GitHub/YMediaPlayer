#include "stdafx.h"
#include "YBaseWindow.h"


YBaseWindow::YBaseWindow()
	:close_func_(nullptr)
	,final_message_func_(nullptr)
{

}


YBaseWindow::~YBaseWindow()
{
}

void YBaseWindow::ShowWindow(bool bShow /* = true */, bool bTakeFocus /* = true */)
{
	::ShowWindow(m_hWnd, bShow ? SW_SHOW : SW_HIDE);
	::SwitchToThisWindow(m_hWnd, TRUE);
}

LRESULT YBaseWindow::OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
	if (close_func_)
	{
		bool result = close_func_();
		if(result)
			::DestroyWindow(GetHWND());
		return result;
	}
	::DestroyWindow(GetHWND());
	return true;
}

LRESULT YBaseWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT result = __super::OnCreate(uMsg, wParam, lParam, bHandled);

	OnAfterCreate();
	return result;
}

LRESULT YBaseWindow::OnDestroy(UINT umsg, WPARAM wp, LPARAM lp, BOOL& bHandled)
{
 	return __super::OnDestroy(umsg, wp, lp, bHandled);
}

void YBaseWindow::OnFinalMessage(HWND hWnd)
{
	if(final_message_func_)
		final_message_func_();
	delete this;
}

LRESULT YBaseWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return __super::HandleCustomMessage(uMsg, wParam, lParam, bHandled);
}

//LRESULT QAQBaseWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//{
//	LRESULT result = __super::OnSize(uMsg, wParam, lParam, bHandled);
//	//auto *ctl_restore = m_PaintManager.FindControl(_T("restorebtn"));
//	//auto *ctl_max = m_PaintManager.FindControl(_T("maxbtn"));
//	//switch (wParam)
//	//{
//	//case SIZE_MINIMIZED:
//	//{
//	//	//ShowWindow(SW_SHOW);
//	//	//RECT re;
//	//	//GetWindowRect(m_hWnd, &re);
//	//	//SetWindowPos(m_hWnd, NULL, 0, 0, 1024, 1024, NULL);
//
//	//	//SetWindowPos(m_hWnd,NULL,-3200,-3200,)
//	//	break;
//	//}
//	//case SIZE_MAXIMIZED:
//	//{
//	//	if (ctl_restore)
//	//		ctl_restore->SetVisible(true);
//	//	if (ctl_max)
//	//		ctl_max->SetVisible(false);
//	//	break;
//	//}
//	//case SIZE_RESTORED:
//	//{
//	//	if (ctl_restore)
//	//		ctl_restore->SetVisible(false);
//	//	if (ctl_max)
//	//		ctl_max->SetVisible(true);
//	//	break;
//	//}
//	//default:
//	//	break;
//	//}
//
//	return result;
//}





LPCTSTR YBaseWindow::GetWindowClassName() const
{
	return L"QAQGameBaseWnd";
}

LRESULT YBaseWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return __super::HandleMessage(uMsg, wParam, lParam);
}

DuiLib::CDuiString YBaseWindow::GetSkinFolder()
{
	throw std::logic_error("The method or operation is not implemented.");
	return L"";
}

DuiLib::CDuiString YBaseWindow::GetSkinFile()
{
	throw std::logic_error("The method or operation is not implemented.");
	return L"";
}
