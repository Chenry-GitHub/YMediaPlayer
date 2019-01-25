#pragma once
#include "BasePlatform.h"
#if PLATFORM_IS_WIN32

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

#endif //PLATFORM_IS_WIN32