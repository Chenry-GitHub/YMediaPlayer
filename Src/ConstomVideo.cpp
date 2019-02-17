#include "ConstomVideo.h"
#if PLATFORM_IS_WIN32


ConstomVideo::ConstomVideo()
{

}


ConstomVideo::~ConstomVideo()
{
}

void ConstomVideo::PlayThread()
{
	while (false == is_stop_)
	{
		
		seek_func_();

		if (!IsPlaying())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		char *data;
		int width;
		int height;
		double pts;
		if (!data_func_(&data, &width, &height, &pts))
		{
			continue;
		}

		clock_ = pts;
		
		if (sync_func_())
		{
			if (user_display_func_)
			{
				user_display_func_(data, width, height);
			}
		}
	}
}

#endif