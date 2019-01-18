#pragma  once
/*
@brief:Player was  developed which based on OpenAV 
@author:yantao
@time:2018-12-19 13:29:48
@details: ʹ��AudioQueΪ���ǵ���Ҫ������ƣ������Ƶ������һ��������-������ģ�ͣ������ˣ���Ƶ�ټ�����ӽ��룬��Ƶ�����̻߳ᱻ����ס��
��������һ�����⣬�ڲ��Ų����ѵ�ʱ�����û�������ߣ������̻߳�һֱ��ס������������ǽ��������ҲҪpush���У�֪ͨ�ϲ����ִ�С�
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