#pragma once
#include "BaseVideo.h"
#include <windows.h>

class GDIVideo :public BaseVideo
{
public:
	GDIVideo();
	~GDIVideo();

	void SetHwnd(HWND hwnd)
	{
		handle_ = hwnd;
	}

	virtual void SetDisplay(void *) override;

protected:
	virtual void PlayThread() override;

	void ShowRGBToWnd(HWND hWnd, BYTE* data, int width, int height);

	HWND handle_;
};

