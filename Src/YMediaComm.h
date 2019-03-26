#pragma once
#include "BasePlatform.h"

enum VideoPlayMode
{
	MODE_USER,

};

enum AudioPlayMode
{
#if PLATFORM_IS_WIN32
	MODE_WIN_WAV,
#endif
#ifdef USE_OPENAL
	MODE_OPENAL,
#endif
};

/*
this is for player to use
*/
enum class PlayerStatus
{
	Stop = 0,
	Pause,
	Playing,
	Buffering,
	Done,
	ErrorDecode,
	ErrorUrl,
	ErrorFormat,
	ErrorTimeOut,
	ErrorUserInterrupt,
};


struct MediaInfo
{
	double dur;

};

/*
	for YMediaDecode.h using
*/
namespace ymc //YMediaCore
{
	enum DecodeError
	{
		ERROR_NO_ERROR = 0,
		ERROR_FORMAT = 0x01,
		ERROR_PKG_ERROR = 0x02,
		ERROR_READ_TIME_OUT = 0x04,
		ERROR_READ_USER_INTERRUPT = 0x08,
	};
}




struct VideoPackageInfo
{
	void *data = nullptr;
	int width=0;
	int height=0;
	double pts=0.0f;
	ymc::DecodeError error = ymc::ERROR_NO_ERROR;
};


struct AudioPackageInfo
{
	void *data = nullptr;
	int size = 0;
	double pts=0.0f;
	ymc::DecodeError error = ymc::ERROR_NO_ERROR;
};


//flag:0 no error
//flag:1 conduct block queue
//用于鉴别该包的状态是否可用
enum FLAG_PKG {
	FLAG_PLAY = 0,
	FLAG_CONDUCT_QUE,
	FLAG_FLUSH_DECODEC,
};


