#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@time:2018-12-19 13:29:48
@details: 使用AudioQue为我们的主要解码控制，如果音频队列是一个消费者-生产者模型，出队了，音频再继续入队解码，音频解码线程会被阻塞住，
这样存在一个问题，在播放层消费的时候，如果没有生产者，播放线程会一直卡住，解决方法就是解码出错了也要push队列，通知上层继续执行。
*/

#ifdef YMEDIA_EXPORT
#define YMEDIA_DECL __declspec(dllexport)
#else
#define YMEDIA_DECL __declspec(dllimport)
#endif

#include <string>
#include <functional>

#include "YMediaComm.h"

class YMediaDecode;
class BaseAudio;
class BaseVideo;
class YMEDIA_DECL YMediaPlayer
{
public:
	YMediaPlayer(AudioPlayMode audio_mode= MODE_OPENAL,VideoPlayMode video_mode=MODE_WIN_GDI);
	~YMediaPlayer();
	
	bool SetMediaFromFile(const std::string & path_file);
	
	bool Play();

	bool Pause();

	bool Stop();

	void Seek(float pos);

	void SetDisplayWindow(void*);

	void SetDurationChangedFunction(std::function<void(int dur)> func);

	void SetCurrentChangedFucnton(std::function<void(int cur)> func);

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

	std::string path_file_;

	MediaInfo media_info_;
	YMediaDecode  *decoder_;

	
	//this is for synchronization
	std::function<void(PlayerStatus)> status_func_;

	std::function<void(int)> dur_func_;
	std::function<void(int)> cur_func_;


	BaseAudio * audio_;
	BaseVideo * video_;


};