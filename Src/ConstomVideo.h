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

protected:
	virtual void PlayThread() override;

};

#endif //PLATFORM_IS_WIN32