#include "YMediaPlayer.h"

#include "YMediaPlayerImp.h"

YMediaPlayer* CreatePlayer(AudioPlayMode audio_mode, VideoPlayMode video_mode, void* opaque)
{
	auto player = new YMediaPlayerImp(audio_mode, video_mode);
	player->SetOpaque(opaque);
	return player;
}

void DeletePlayer(YMediaPlayer*player)
{
	if(player)
		delete player;
}
