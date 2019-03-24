#pragma once
#include "YMediaComm.h"
#ifdef YMEDIA_EXPORT
#define YMEDIA_DECL __declspec(dllexport)
#else
#define YMEDIA_DECL __declspec(dllimport)
#endif
using DurFunc = void(*)(void *opaque,int);
using CurFunc = void(*)(void *opaque,int);
using StatusFunc = void(*)(PlayerStatus);
using VideoFunc = void(*)(void *opaque, void *data,int width,int height);
using BufferFunc = void(*)(void *opaque, float percent);

class YMediaPlayer
{
public:
	virtual ~YMediaPlayer() {};

	virtual bool SetMedia(const char* path_file) =0;

	virtual bool Play() = 0;

	virtual bool Pause() = 0;

	virtual bool IsPlaying() = 0;

	virtual bool Stop() = 0;

	virtual void Seek(float pos) = 0;

	virtual void SetOpaque(void*)=0;

	virtual void SetDurationChangedFunction(DurFunc func) = 0;

	virtual void SetCurrentChangedFucnton(CurFunc func) = 0;

	virtual void SetUserHandleVideoFunction(VideoFunc func) = 0;

	virtual void SetBufferFunction(BufferFunc func) = 0;
};

YMEDIA_DECL YMediaPlayer* CreatePlayer(AudioPlayMode audio_mode, VideoPlayMode video_mode,void* opaque);

YMEDIA_DECL void DeletePlayer(YMediaPlayer*);