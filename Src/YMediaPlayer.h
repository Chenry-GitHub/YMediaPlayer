#pragma once
#include "YMediaComm.h"
#ifdef YMEDIA_EXPORT
#define YMEDIA_DECL __declspec(dllexport)
#else
#define YMEDIA_DECL __declspec(dllimport)
#endif


class YMediaPlayer
{
public:
	class Delegate
	{
	public:
		virtual void onDurationChanged(YMediaPlayer* , int duration) = 0;
		virtual void onCurrentChanged(YMediaPlayer*, int changed) = 0;
		virtual void onStatusChanged(YMediaPlayer*, PlayerStatus status) = 0;
		virtual void onVideoData(YMediaPlayer*,void *data,int width,int height) = 0;
		virtual void onNetworkBuffer(YMediaPlayer* ,float percent)=0;
	};

	virtual ~YMediaPlayer() {};

	virtual void setDelegate(YMediaPlayer::Delegate* dele) = 0 ;

	virtual YMediaPlayer::Delegate* getDelegate() = 0; 

	virtual bool setMedia(const char* path_file) =0;

	virtual bool play() = 0;

	virtual bool pause() = 0;

	virtual bool isPlaying() = 0;

	virtual bool stop() = 0;

	virtual void seek(float pos) = 0;

	virtual void setOpaque(void*) = 0;

	virtual void* getOpaque() = 0;
};

YMEDIA_DECL YMediaPlayer* CreatePlayer(AudioPlayMode audio_mode, VideoPlayMode video_mode);

YMEDIA_DECL void DeletePlayer(YMediaPlayer*);