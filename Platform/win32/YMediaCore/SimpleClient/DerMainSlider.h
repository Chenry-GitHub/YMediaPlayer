#pragma once

class DerMainSlider :
	public DuiLib::CControlUI
{
public:
	DerMainSlider();
	~DerMainSlider();

	static CDuiString classname_;
	float percent_=0.0f;

	void SetBufferPercent(float percent)
	{
		percent_ = percent;
		Invalidate();
	}


	void SetRange(int min, int max)
	{
		max_value_ = max;
		min_value_ = min;
	}
	void SetCurValue(int value)
	{
		cur_value_ = value;
		Invalidate();
	}

	void SetBuffer()
	{

	}



	virtual bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl) override;


	virtual LPCTSTR GetClass() const override;


	virtual void DoEvent(TEventUI& event) override;

	std::function<void (float)> clicked_func_=nullptr;
protected:


	int cur_value_ = 0;
	int max_value_ = 0;
	int min_value_ = 0;

};

