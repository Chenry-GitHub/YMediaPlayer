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

#include "..\HttpDownload.h"
#include "..\StreamIOMgr.h"


class YMediaDecode;
class BaseAudio;
class BaseVideo;
class YMediaPlayerImp:public YMediaPlayer
{
public:
	YMediaPlayerImp(AudioPlayMode audio_mode,VideoPlayMode video_mode);
	virtual~YMediaPlayerImp() override;
	
	virtual bool SetMedia(const char* path_file) override;
	
	virtual bool Play() override;

	virtual bool Pause() override;

	virtual bool IsPlaying() override;

	virtual bool Stop() override;

	virtual void Seek(float pos) override;


	virtual void SetDurationChangedFunction(DurFunc func) override;

	virtual void SetCurrentChangedFucnton(CurFunc func) override;

	virtual void SetOpaque(void*) override;

	virtual void SetUserHandleVideoFunction(VideoFunc func) override;


	virtual void SetBufferFunction(BufferFunc func) override;

protected:
	void OnAudioDataFree(char *data);

	bool OnSynchronizeVideo();

	void OnDecodeError(DecodeError error);

	void OnMediaInfo(MediaInfo info);

	bool OnAudioDataFunction(char ** data, int *len,double *pts);

	bool OnVideoDataFunction(char ** data, int *width, int *height, double *pts);

	bool OnAudioSeekFunction();

	bool OnVideoSeekFunction();

	bool OnUserDisplayFunction(void *data,int width,int height);

	int OnReadMem(char*data,int len);

	int64_t OnSeekMem(int64_t offset, int whence);
private:
	void NotifyPlayerStatus(PlayerStatus);

	const char * path_file_;

	MediaInfo media_info_;
	YMediaDecode  *decoder_;

	
	int64_t cur_pos_ = 0;

	//this is for synchronization
	StatusFunc status_func_;

	DurFunc dur_func_;
	CurFunc cur_func_;
	VideoFunc user_video_func_;
	BufferFunc buffer_func_;

	BaseAudio * audio_;
	BaseVideo * video_;

	void *opaque_;

	HttpDownload  network_ ;

	StreamIOMgr io_mgr_;
};