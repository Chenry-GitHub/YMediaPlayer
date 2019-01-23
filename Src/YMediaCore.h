#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@email:what951006@163.com
@time:2018-12-19 13:29:48
@details: ʹ��AudioQueΪ���ǵ���Ҫ������ƣ���Ƶ������һ��������-������ģ�ͣ������ˣ���Ƶ�ټ�����ӽ��룬��Ƶ�����̻߳ᱻ����ס��
��������һ�����⣬�ڲ��Ų����ѵ�ʱ�����û�������ߣ������̻߳�һֱ��ס������������ǽ��������ҲҪpush���У�֪ͨ�ϲ����ִ�С�
*/

#ifdef YMEDIA_EXPORT
#define YMEDIA_DECL __declspec(dllexport)
#else
#define YMEDIA_DECL __declspec(dllimport)
#endif

#include "YMediaComm.h"

using DurFunc = void (*)(int);
using CurFunc = void (*)(int);
using StatusFunc = void (*)(PlayerStatus);


class YMediaDecode;
class BaseAudio;
class BaseVideo;
class YMEDIA_DECL YMediaPlayer
{
public:
	YMediaPlayer(AudioPlayMode audio_mode= MODE_OPENAL,VideoPlayMode video_mode=MODE_WIN_GDI);
	~YMediaPlayer();
	
	bool SetMediaFromFile(const char* path_file);
	
	bool Play();

	bool Pause();

	bool IsPlaying();

	bool Stop();

	void Seek(float pos);

	void SetDisplayWindow(void*);

	void SetDurationChangedFunction(DurFunc func);

	void SetCurrentChangedFucnton(CurFunc func);

protected:

	bool OnSynchronizeVideo();

	void OnDecodeError(DecodeError error);

	void OnMediaInfo(MediaInfo info);

	bool OnAudioDataFunction(char ** data, int *len,double *pts);

	bool OnVideoDataFunction(char ** data, int *width, int *height, double *pts);

	bool OnAudioSeekFunction();

	bool OnVideoSeekFunction();
private:
	void NotifyPlayerStatus(PlayerStatus);

	const char * path_file_;

	MediaInfo media_info_;
	YMediaDecode  *decoder_;

	
	//this is for synchronization
	StatusFunc status_func_;

	DurFunc dur_func_;
	CurFunc cur_func_;


	BaseAudio * audio_;
	BaseVideo * video_;


};