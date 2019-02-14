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
#include <fstream>



class YMediaDecode;
class BaseAudio;
class BaseVideo;
class YMediaPlayerImp:public YMediaPlayer
{
public:
	YMediaPlayerImp(AudioPlayMode audio_mode,VideoPlayMode video_mode);
	virtual ~YMediaPlayerImp();
	
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
	void OnAudioDataFree(char *data);

	bool OnSynchronizeVideo();

	void OnDecodeError(DecodeError error);

	void OnMediaInfo(MediaInfo info);

	bool OnAudioDataFunction(char ** data, int *len,double *pts);

	bool OnVideoDataFunction(char ** data, int *width, int *height, double *pts);

	bool OnAudioSeekFunction();

	bool OnVideoSeekFunction();

	int OnReadMem(char*data,int len);

	int64_t OnSeekMem(void *opaque, int64_t offset, int whence);
private:
	void NotifyPlayerStatus(PlayerStatus);

	const char * path_file_;

	MediaInfo media_info_;
	YMediaDecode  *decoder_;
	std::ifstream read_fs_;
	
	//this is for synchronization
	StatusFunc status_func_;

	DurFunc dur_func_;
	CurFunc cur_func_;


	BaseAudio * audio_;
	BaseVideo * video_;
};