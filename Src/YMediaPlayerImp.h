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