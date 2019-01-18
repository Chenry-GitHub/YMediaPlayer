#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@time:2018-12-19 13:29:48
@details: 使用AudioQue为我们的主要解码控制，如果音频队列是一个消费者-生产者模型，出队了，音频再继续入队解码，音频解码线程会被阻塞住，
这样存在一个问题，在播放层消费的时候，如果没有生产者，播放线程会一直卡住，解决方法就是解码出错了也要push队列，通知上层继续执行。
*/
#include "YMediaDecode.h"
#include "BaseAudio.h"
#include "BaseVideo.h"

#include <string>
#include <functional>


class YMediaPlayer
{
public:
	YMediaPlayer();
	~YMediaPlayer();

	bool SetMediaFromFile(const std::string & path_file);

//	bool SetMediaFromUrl(const std::string & stream_url);

	bool Play();

	bool Pause();

	bool Stop();


	void Seek(float pos);
protected:


	void synchronize_video();

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
	YMediaDecode  decoder_;

	
	//this is for synchronization
	std::function<void(PlayerStatus)> status_func_;


	BaseAudio * audio_;
	BaseVideo * video_;


};