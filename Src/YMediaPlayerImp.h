#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@email:what951006@163.com
@time:2018-12-19 13:29:48
@details: 使用AudioQue为我们的主要解码控制，音频队列是一个消费者-生产者模型，出队了，音频再继续入队解码，音频解码线程会被阻塞住，
这样存在一个问题，在播放层消费的时候，如果没有生产者，播放线程会一直卡住，解决方法就是解码出错了也要push队列，通知上层继续执行。
*/


#include "YMediaPlayer.h"
#include "YMediaDecode.h"
#include "BaseAudio.h"
#include "BaseVideo.h"


class YMediaDecode;
class BaseAudio;
class BaseVideo;
class YMediaPlayerImp:public YMediaPlayer
					,public YMediaDecode::Delegate
					,public BaseAudio::Delegate
					,public BaseVideo::Delegate
{
public:
	YMediaPlayerImp(AudioPlayMode audio_mode,VideoPlayMode video_mode);
	virtual~YMediaPlayerImp() override;
	virtual void setDelegate(YMediaPlayer::Delegate* dele) override;

	virtual YMediaPlayer::Delegate* getDelegate() override;

	virtual bool setMedia(const char* path_file) override;
	
	virtual bool play() override;

	virtual bool pause() override;

	virtual bool isPlaying() override;

	virtual bool stop() override;

	virtual void seek(float pos) override;

	virtual void setOpaque(void*) override;

	virtual void* getOpaque() override;

//
protected:

	virtual void onDecodeError(ymc::DecodeError) override;
	virtual void onMediaInfo(MediaInfo) override;

	virtual bool onVideoSeek() override;
	virtual bool onVideoGetData(char ** data, int *width, int *height, double *pts) override;
	virtual bool onVideoSync() override;
	virtual void onVideoDisplay(void *data, int width, int height) override;
	
	
	virtual bool onAudioGetData(char ** data, int *len, double *pts) override;
	virtual void onAudioCurrent(int) override;
	virtual void onAudioSeek() override;
	virtual void onAudioFreeData(char*) override;
protected:

private:
	void notifyPlayerStatus(PlayerStatus);

	MediaInfo media_info_;
	YMediaDecode decoder_;


	YMediaPlayer::Delegate* player_delegate_ = nullptr;
	BaseAudio * audio_ = nullptr;
	BaseVideo * video_ = nullptr;
	void *opaque_ = nullptr;
};