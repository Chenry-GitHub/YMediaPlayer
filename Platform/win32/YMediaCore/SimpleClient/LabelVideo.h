#pragma once
class LabelVideo :
	public DuiLib::CControlUI
{
public:
	LabelVideo();
	~LabelVideo();

	void SetImage(void * data ,int w,int h);

	static const CDuiString classname_;

	virtual LPCTSTR GetClass() const override;

protected:
	virtual bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl) override;

	Gdiplus::Bitmap *pixmap_ = nullptr;
};

