#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@time:2018-12-19 13:29:48
*/
#include <al.h>
#include <alc.h>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "YMediaDecode.h"

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


	void FillAudioBuff(ALuint& buf);
protected:
	int  PlayThread();


private:
	std::string path_file_;
	ALuint		source_id_;

	std::unique_ptr<YMediaDecode *> decoder_ptr_;

	std::thread play_thread_;

	atomic_bool is_need_stop_;

	ALuint audio_buf_[NUMBUFFERS];
};