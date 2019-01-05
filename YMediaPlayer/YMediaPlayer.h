#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@time:2018-12-19 13:29:48
@details: 使用AudioQue为我们的主要解码控制，如果音频队列是一个消费者-生产者模型，出队了，音频再继续入队解码，音频解码线程会被阻塞住，
这样存在一个问题，在播放层消费的时候，如果没有生产者，播放线程会一直卡住，解决方法就是解码出错了也要push队列，通知上层继续执行。
*/
#include <al.h>
#include <alc.h>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <glew.h>
#include <glfw3.h>
#include <gl/GL.h>

#include "YMediaDecode.h"
//#include "Shader.h"

#define NUMBUFFERS              8
 
class YMediaPlayer
{
public:
	YMediaPlayer();
	~YMediaPlayer();
	static int InitPlayer();
	static int UnInitPlayer();

	bool SetMediaFromFile(const std::string & path_file);

//	bool SetMediaFromUrl(const std::string & stream_url);

	bool Play();

	bool Pause();

	bool Stop();

	bool IsPause();

	YMediaPlayerError FillAudioBuff(ALuint& buf);
protected:
	int  AudioPlayThread();

	int VideoPlayThread();
private:
	std::string path_file_;
	ALuint		source_id_;

	YMediaDecode  decoder_;

	std::thread audio_thread_;
	std::thread video_thread_;

	atomic_bool is_need_stop_;

	atomic_bool is_pause_;

	ALuint audio_buf_[NUMBUFFERS];

	//为第一次执行
	atomic_bool is_prepare_;
	
	//this is for synchronization
	int audio_pts;
	int video_pts;

};