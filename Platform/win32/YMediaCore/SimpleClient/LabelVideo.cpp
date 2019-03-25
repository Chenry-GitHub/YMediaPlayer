#include "stdafx.h"
#include "LabelVideo.h"

const CDuiString LabelVideo::classname_ = L"LabelVideo";
const int timer_id = 24;

LPCTSTR LabelVideo::GetClass() const
{
	return LabelVideo::classname_;
}

void LabelVideo::DoEvent(TEventUI& event)
{
	if (event.Type == UIEVENT_TIMER && event.wParam == timer_id) {
		//std::lock_guard<std::mutex> lg(mutex_);
		Invalidate();
	}
}

LabelVideo::LabelVideo()
{
	
}


LabelVideo::~LabelVideo()
{
}

void LabelVideo::SetImage(void * data, int w, int h)
{
	pixmap_ = new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)data);
	Invalidate();

}

void LabelVideo::StartTimer()
{
	GetManager()->SetTimer(this, timer_id, 30);
}

void LabelVideo::StopTimer()
{
	GetManager()->KillTimer(this, timer_id);
}

bool LabelVideo::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
	std::lock_guard<std::mutex> lg(mutex_);
	Gdiplus::Graphics graphics(hDC);
	graphics.DrawImage(pixmap_, rcPaint.left, rcPaint.top, GetWidth(), GetHeight());
	if (pixmap_)
	{
		delete pixmap_;
		pixmap_ = nullptr;
	}

	return true;//__super::DoPaint(hDC, rcPaint, pStopControl);
}
