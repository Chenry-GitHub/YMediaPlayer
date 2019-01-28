#include "YMediaPlayer.h"

#include "YMediaPlayerImp.h"

YMediaPlayer* CreatePlayer(AudioPlayMode audio_mode, VideoPlayMode video_mode)
{
	return new YMediaPlayerImp(audio_mode, video_mode);
}

void DeletePlayer(YMediaPlayer*player)
{
	if(player)
		delete player;
}


