#pragma once
#include <mutex>

class LabelVideo :
	public DuiLib::CControlUI
{
public:
	LabelVideo();
	~LabelVideo();

	void SetImage(void * data ,int w,int h);

	void StartTimer();

	void StopTimer();


	static const CDuiString classname_;

	virtual LPCTSTR GetClass() const override;


	virtual void DoEvent(TEventUI& event) override;

protected:
	virtual bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl) override;

	Gdiplus::Bitmap *pixmap_ = nullptr;

	std::mutex mutex_;
};

