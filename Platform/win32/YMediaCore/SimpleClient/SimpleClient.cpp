// SimpleClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SimpleClient.h"
#include "MainWindow.h"
#include "MainMgr.h"



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	CPaintManagerUI::SetInstance(GetModuleHandle(NULL));

	MainMgr mgr;
	mgr.Init();

	CPaintManagerUI::MessageLoop();
	CPaintManagerUI::Term();
	return 0;
}

