#include "YMediaPlayerImp.h"


#include "BaseAudio.h"
#include "BaseVideo.h"
#include "YMediaDecode.h"

#include "OpenALAudio.h"
#include "OpenGLVideo.h"
#if PLATFORM_IS_WIN32
#include "GDIVideo.h"
#include "WavAudio.h"
#endif


YMediaPlayerImp::YMediaPlayerImp(AudioPlayMode audio_mode, VideoPlayMode video_mode)
	:status_func_(nullptr)
	, opaque_(nullptr)
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
#if PLATFORM_IS_WIN32
		case MODE_WIN_WAV:
		{
			audio_ = new WavAudio();

			break;
		}
#endif
	}
	audio_->SetDataFunction(std::bind(&YMediaPlayerImp::OnAudioDataFunction,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	audio_->SetBlockSeekFunction(std::bind(&YMediaPlayerImp::OnAudioSeekFunction, this));
	audio_->SetFreeDataFunction(std::bind(&YMediaPlayerImp::OnAudioDataFree,this, std::placeholders::_1));


	//this is for video mode
	switch (video_mode)
	{
#if PLATFORM_IS_WIN32
	case MODE_WIN_GDI:
	{
		video_ = new GDIVideo();
		break;
	}
#endif
	case MODE_OPENGL:
		video_ = new OpenGLVideo();
		break;
	}
	video_->SetDataFunction(std::bind(&YMediaPlayerImp::OnVideoDataFunction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	video_->SetSyncToAudioFunction(std::bind(&YMediaPlayerImp::OnSynchronizeVideo, this));
	video_->SetBlockSeekFunction(std::bind(&YMediaPlayerImp::OnVideoSeekFunction, this));


	decoder_ = new YMediaDecode();
	decoder_->SetErrorFunction(std::bind(&YMediaPlayerImp::OnDecodeError,this, std::placeholders::_1));
	decoder_->SetMediaFunction(std::bind(&YMediaPlayerImp::OnMediaInfo, this, std::placeholders::_1));

}

YMediaPlayerImp::~YMediaPlayerImp()
{
	Stop();

}


bool YMediaPlayerImp::SetMediaFromFile(const char* path_file)
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

bool YMediaPlayerImp::Play()
{
	audio_->Play();
	video_->Play();
	return true;
}

bool YMediaPlayerImp::Pause()
{
	audio_->Pause();
	video_->Pause();
	return true;
}

bool YMediaPlayerImp::IsPlaying()
{
	return audio_->IsPlaying();
}

bool YMediaPlayerImp::Stop()
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



void YMediaPlayerImp::Seek(float pos)
{
	decoder_->SeekPos(media_info_.dur*pos);
	audio_->Seek(pos);
	video_->Seek(pos);
}

void YMediaPlayerImp::SetDisplayWindow(void* handle)
{
	video_->SetDisplay(handle);
}

void YMediaPlayerImp::SetDurationChangedFunction(DurFunc func)
{
	dur_func_ = func;
}

void YMediaPlayerImp::SetCurrentChangedFucnton(CurFunc func)
{
	cur_func_ = func;
	audio_->SetProgressFunction([this](int dur) {
		cur_func_(opaque_,dur);
	});
}

void YMediaPlayerImp::SetOpaque(void*opa)
{
	opaque_ = opa;
}

void YMediaPlayerImp::OnAudioDataFree(char *data)
{
	if (data)
	{
		AudioPackageInfo info;
		info.data = data;
		info.error = ERROR_NO_ERROR;
		decoder_->FreeAudioPackageInfo(&info);
	}
	
}

bool YMediaPlayerImp::OnSynchronizeVideo()
{
	while (!audio_->IsStop())
	{
		//printf("%f,%f \n", video_->GetClock(), audio_->GetClock());
		if (video_->GetClock()<= audio_->GetClock())
			return true;
		int delayTime = static_cast<int>((video_->GetClock() - audio_->GetClock()) * 1000);
		delayTime = delayTime > 1 ? 1 : delayTime;
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
	}
	return false;
}

void YMediaPlayerImp::OnDecodeError(DecodeError error)
{
	printf("OnDecodeError  finished %d\n", error);
}

void YMediaPlayerImp::OnMediaInfo(MediaInfo info)
{
	media_info_ = info;
	audio_->SetDuration(info.dur);
	video_->SetDuration(info.dur);
	
	if(dur_func_)
		dur_func_(opaque_ , (int)info.dur);
	printf("OnMediaInfo :Dur-%f,\n", media_info_.dur);
}

bool YMediaPlayerImp::OnAudioDataFunction(char ** data, int *len, double *pts)
{
	AudioPackageInfo &&info = decoder_->PopAudioQue();
	if (info.error == ERROR_NO_ERROR)
	{
		*data = (char*)info.data;
		*len = info.size;
		*pts = info.pts;
		return true;
	}
	
	return false;
}

bool YMediaPlayerImp::OnVideoDataFunction(char ** data, int *width, int *height, double *pts)
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

bool YMediaPlayerImp::OnAudioSeekFunction()
{
	return decoder_->JudgeBlockAudioSeek();
}

bool YMediaPlayerImp::OnVideoSeekFunction()
{
	return decoder_->JudgeBlockVideoSeek();
}

int YMediaPlayerImp::OnReadMem(char*data, int len)
{
	
	return len;
}

void YMediaPlayerImp::NotifyPlayerStatus(PlayerStatus st)
{
	if (status_func_)
		status_func_(st);
}