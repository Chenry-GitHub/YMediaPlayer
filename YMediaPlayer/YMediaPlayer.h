#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@time:2018-12-19 13:29:48
@details: ʹ��AudioQueΪ���ǵ���Ҫ������ƣ������Ƶ������һ��������-������ģ�ͣ������ˣ���Ƶ�ټ�����ӽ��룬��Ƶ�����̻߳ᱻ����ס��
��������һ�����⣬�ڲ��Ų����ѵ�ʱ�����û�������ߣ������̻߳�һֱ��ס������������ǽ��������ҲҪpush���У�֪ͨ�ϲ����ִ�С�
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

	//Ϊ��һ��ִ��
	atomic_bool is_prepare_;
	
	//this is for synchronization
	int audio_pts;
	int video_pts;

};