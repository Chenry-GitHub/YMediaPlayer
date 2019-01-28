#pragma once
#include "YMediaComm.h"
#ifdef YMEDIA_EXPORT
#define YMEDIA_DECL __declspec(dllexport)
#else
#define YMEDIA_DECL __declspec(dllimport)
#endif
using DurFunc = void(*)(int);
using CurFunc = void(*)(int);
using StatusFunc = void(*)(PlayerStatus);

class YMEDIA_DECL YMediaPlayer
{
public:
	//YMediaImplement(AudioPlayMode audio_mode, VideoPlayMode video_mode)=0;
	virtual ~YMediaPlayer() {};

	virtual bool SetMediaFromFile(const char* path_file) =0;

	virtual bool Play() = 0;

	virtual bool Pause() = 0;

	virtual bool IsPlaying() = 0;

	virtual bool Stop() = 0;

	virtual void Seek(float pos) = 0;

	virtual void SetDisplayWindow(void*) = 0;

	virtual void SetDurationChangedFunction(DurFunc func) = 0;

	virtual void SetCurrentChangedFucnton(CurFunc func) = 0;

};

YMEDIA_DECL YMediaPlayer* CreatePlayer(AudioPlayMode audio_mode, VideoPlayMode video_mode);

YMEDIA_DECL void DeletePlayer(YMediaPlayer*);