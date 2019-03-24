#include "stdafx.h"
#include "LabelVideo.h"

const CDuiString LabelVideo::classname_ = L"LabelVideo";

LPCTSTR LabelVideo::GetClass() const
{
	return LabelVideo::classname_;
}

LabelVideo::LabelVideo()
{

}


LabelVideo::~LabelVideo()
{
}

void LabelVideo::SetImage(void * data, int w, int h)
{
	if (!data || w <= 0 || h <= 0)
		return;

	if (pixmap_)
		delete pixmap_;
	pixmap_ = new Gdiplus::Bitmap(w, h, w * 4, PixelFormat32bppPARGB, (BYTE*)data);
	Invalidate();
}

bool LabelVideo::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
	
	Gdiplus::Graphics graphics(hDC);
	graphics.DrawImage(pixmap_, rcPaint.left, rcPaint.top, GetWidth(), GetHeight());

	return true;//__super::DoPaint(hDC, rcPaint, pStopControl);
}
