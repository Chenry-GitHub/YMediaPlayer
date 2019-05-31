#pragma once
#include "BasePlatform.h"
#if PLATFORM_IS_WIN32

#include "BaseVideo.h"
#include <windows.h>

class ConstomVideo :public BaseVideo
{
public:
	ConstomVideo();
	~ConstomVideo();


	virtual void setDelegate(BaseVideo::Delegate * dele) override;


	virtual BaseVideo::Delegate * getDelegate() override;

protected:
	virtual void PlayThread() override;

	BaseVideo::Delegate * delegate_ = nullptr;
};

#endif //PLATFORM_IS_WIN32