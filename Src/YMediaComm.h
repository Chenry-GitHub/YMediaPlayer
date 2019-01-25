#pragma once

enum VideoPlayMode
{
	MODE_WIN_GDI,
	MODE_OPENGL,
};

enum AudioPlayMode
{
	MODE_WIN_WAV,
	MODE_OPENAL,
};


struct PlayerStatus
{
	enum Status
	{
		Stop = 0,
		Pause,
		Playing,
		During,
		Done,
		FileError,
	};
	Status status = Pause;
};


struct MediaInfo
{
	double dur;

};

enum YMediaCallBackType
{
	MEDIA_ERROR,
};

enum DecodeError
{
	ERROR_NO_ERROR = 0,
	ERROR_NO_QUIT,
	ERROR_FILE_ERROR,
	ERROR_PKG_ERROR,
};


struct VideoPackageInfo
{
	void *data = nullptr;
	int width=0;
	int height=0;
	double pts=0.0f;
	DecodeError error = ERROR_NO_ERROR;
};


struct AudioPackageInfo
{
	void *data = nullptr;
	int size = 0;
	double pts=0.0f;
	DecodeError error = ERROR_NO_ERROR;
};


//flag:0 no error
//flag:1 conduct block queue
//用于鉴别该包的状态是否可用
enum FLAG_PKG {
	FLAG_PLAY = 0,
	FLAG_CONDUCT_QUE,
	FLAG_FLUSH_DECODEC,
};


