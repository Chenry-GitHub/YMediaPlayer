#include "YMediaCore.h"


#include "BaseAudio.h"
#include "BaseVideo.h"
#include "YMediaDecode.h"

#include "OpenALAudio.h"
#include "OpenGLVideo.h"
#include "GDIVideo.h"
#include "WavAudio.h"


YMediaPlayer::YMediaPlayer(AudioPlayMode audio_mode, VideoPlayMode video_mode)
	:status_func_(nullptr)
{
	//this is for initialize audio
	switch (audio_mode)
	{
		case MODE_OPENAL:
		{
			OpenALAudio::InitPlayer();
			audio_ = new OpenALAudio();
			break;
		}
		case MODE_WIN_WAV:
		{
			audio_ = new WavAudio();

			break;
		}
	}
	audio_->SetDataFunction(std::bind(&YMediaPlayer::OnAudioDataFunction,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	audio_->SetBlockSeekFunction(std::bind(&YMediaPlayer::OnAudioSeekFunction, this));

	//this is for video mode
	switch (video_mode)
	{
	case MODE_WIN_GDI:
		video_ = new GDIVideo();
		break;
	case MODE_OPENGL:
		video_ = new OpenGLVideo();
		break;
	}
	video_->SetDataFunction(std::bind(&YMediaPlayer::OnVideoDataFunction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	video_->SetSyncToAudioFunction(std::bind(&YMediaPlayer::OnSynchronizeVideo, this));
	video_->SetBlockSeekFunction(std::bind(&YMediaPlayer::OnVideoSeekFunction, this));


	decoder_ = new YMediaDecode();
	decoder_->SetErrorFunction(std::bind(&YMediaPlayer::OnDecodeError,this, std::placeholders::_1));
	decoder_->SetMediaFunction(std::bind(&YMediaPlayer::OnMediaInfo, this, std::placeholders::_1));
}

YMediaPlayer::~YMediaPlayer()
{
	Stop();

}


bool YMediaPlayer::SetMediaFromFile(const std::string & path_file)
{
	Stop();
	printf("Stop\n");

	path_file_ = path_file;

	decoder_->SetMedia(path_file, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL);
	printf("decoder_.SetMedia\n");

	audio_->BeginPlayThread();

	video_->BeginPlayThread();


	printf("SetMediaFromFile\n");
	return true;
}

bool YMediaPlayer::Play()
{
	audio_->Play();
	return true;
}

bool YMediaPlayer::Pause()
{
	audio_->Pause();
	return true;
}

bool YMediaPlayer::Stop()
{
	decoder_->ConductAudioBlocking();
	audio_->Stop();
	audio_->EndPlayThread();

	decoder_->ConductVideoBlocking();
	video_->Stop();
	video_->EndPlayThread();

	decoder_->StopDecode();
	return true;
}



void YMediaPlayer::Seek(float pos)
{
	decoder_->SeekPos(media_info_.dur*pos);
	audio_->Seek(pos);
	video_->Seek(pos);
}

void YMediaPlayer::SetDisplayWindow(void* handle)
{
	video_->SetDisplay(handle);
}

void YMediaPlayer::SetDurationChangedFunction(std::function<void(int dur)> func)
{
	dur_func_ = func;
}

void YMediaPlayer::SetCurrentChangedFucnton(std::function<void(int cur)> func)
{
	cur_func_ = func;
	audio_->SetProgressFunction(func);
}

void YMediaPlayer::OnSynchronizeVideo()
{
	while (false == audio_->IsStop())
	{
		printf("%f,%f \n", video_->GetClock(), audio_->GetClock());
		if (video_->GetClock()<= audio_->GetClock())
			break;
		int delayTime = (video_->GetClock() - audio_->GetClock()) * 1000;
		delayTime = delayTime > 1 ? 1 : delayTime;
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
	}
}

void YMediaPlayer::OnDecodeError(DecodeError error)
{
	//while(audio_thread_runing_)
	//{
	//	decoder_.ConductAudioBlocking();
	//}

	//while(video_thread_runing_)
	//{
	//	decoder_.ConductVideoBlocking();
	//}
	printf("OnDecodeError  finished %d\n", error);
}

void YMediaPlayer::OnMediaInfo(MediaInfo info)
{
	media_info_ = info;
	audio_->SetDuration(info.dur);
	video_->SetDuration(info.dur);
	
	if(dur_func_)
		dur_func_(info.dur);
	printf("OnMediaInfo :Dur-%f,\n", media_info_.dur);
}

bool YMediaPlayer::OnAudioDataFunction(char ** data, int *len, double *pts)
{
	AudioPackageInfo &&info = decoder_->PopAudioQue();
	if (info.error == ERROR_NO_ERROR)
	{
		*data = (char*)info.data;
		*len = info.size;
		*pts = info.pts;
		decoder_->FreeAudioPackageInfo(&info);
		return true;
	}
	
	return false;
}

bool YMediaPlayer::OnVideoDataFunction(char ** data, int *width, int *height, double *pts)
{
	VideoPackageInfo  && pkg = decoder_->PopVideoQue(video_->GetClock());
	if (pkg.error == ERROR_NO_ERROR)
	{
		*data = (char*)pkg.data;
		*width = pkg.width;
		*height = pkg.height;
		*pts = pkg.pts;
		return true;
	}
	return false;
}

bool YMediaPlayer::OnAudioSeekFunction()
{
	return decoder_->JudgeBlockAudioSeek();
}

bool YMediaPlayer::OnVideoSeekFunction()
{
	return decoder_->JudgeBlockVideoSeek();
}

void YMediaPlayer::NotifyPlayerStatus(PlayerStatus st)
{
	if (status_func_)
		status_func_(st);
}