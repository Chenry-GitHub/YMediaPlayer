#include "stdafx.h"
#include "DerMainSlider.h"

CDuiString DerMainSlider::classname_ = L"DerMainSlider";
DerMainSlider::DerMainSlider()
{
}


DerMainSlider::~DerMainSlider()
{
}

bool DerMainSlider::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
	Gdiplus::Graphics graphics(hDC);

	//background
	Gdiplus::Rect re{ GetX(),GetY(),GetWidth(),GetHeight() };
	graphics.FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color(0x7A, 0x80, 0xA2)), re);

	//buffer
	Gdiplus::Rect re2{ GetX(),GetY(),(int)(GetWidth()*percent_),GetHeight() };
	graphics.FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color(0xff, 0x00, 0x00)), re2 );
	
	//value
	Gdiplus::Rect re3{ GetX(),GetY(),(max_value_ - min_value_ == 0 ? 0 :(int)(GetWidth()*((float)cur_value_/(max_value_- min_value_)))),GetHeight() };
	graphics.FillRectangle(&Gdiplus::SolidBrush(Gdiplus::Color(0xFF, 0xe3, 0x36)), re3);

	return true;
}

LPCTSTR DerMainSlider::GetClass() const
{
	return classname_;
}



void DerMainSlider::DoEvent(TEventUI& event)
{
	if (event.Type == UIEVENT_BUTTONDOWN)
	{
		
		float percent = (float)(event.ptMouse.x-GetX()) / GetWidth();
		if (clicked_func_)
			clicked_func_(percent);
		//SetCurValue((max_value_-min_value_)*percent);
	}

	__super::DoEvent(event);
}
