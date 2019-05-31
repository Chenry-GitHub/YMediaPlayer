#include "ConstomVideo.h"
#if PLATFORM_IS_WIN32


ConstomVideo::ConstomVideo()
{

}


ConstomVideo::~ConstomVideo()
{
}

void ConstomVideo::playThread()
{
	while (false == is_stop_)
	{
		
		delegate_->onVideoSeek();

		if (!isPlaying())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		char *data;
		int width;
		int height;
		double pts;
		if (!delegate_->onVideoGetData(&data, &width, &height, &pts))
		{
			continue;
		}

		clock_ = pts;
		
		if (delegate_->onVideoSync())
		{
			delegate_->onVideoDisplay(data, width, height);
		}
	}
}

#endif