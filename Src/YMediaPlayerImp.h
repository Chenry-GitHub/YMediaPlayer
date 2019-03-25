#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@email:what951006@163.com
@time:2018-12-19 13:29:48
@details: ʹ��AudioQueΪ���ǵ���Ҫ������ƣ���Ƶ������һ��������-������ģ�ͣ������ˣ���Ƶ�ټ�����ӽ��룬��Ƶ�����̻߳ᱻ����ס��
��������һ�����⣬�ڲ��Ų����ѵ�ʱ�����û�������ߣ������̻߳�һֱ��ס������������ǽ��������ҲҪpush���У�֪ͨ�ϲ����ִ�С�
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