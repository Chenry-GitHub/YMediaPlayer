#include "YMediaPlayerImp.h"


#include "BaseAudio.h"
#include "BaseVideo.h"
#include "YMediaDecode.h"

#include "OpenALAudio.h"
#include "ConstomVideo.h"
#if PLATFORM_IS_WIN32
#include "WavAudio.h"
#endif


YMediaPlayerImp::YMediaPlayerImp(AudioPlayMode audio_mode, VideoPlayMode video_mode)
{
	//this is for initialize audio
	switch (audio_mode)
	{
#if  USE_OPENAL
		case MODE_OPENAL:
		{
			OpenALAudio::InitPlayer();
			audio_ = new OpenALAudio();
			break;
		}
#endif //USE_OPENAL
#if PLATFORM_IS_WIN32
		case MODE_WIN_WAV:
		{
			audio_ = new WavAudio();

			break;
		}
#endif
	}
	audio_->setDelegate(this);

	//this is for video mode
	switch (video_mode)
	{
	case MODE_USER:
		video_ = new ConstomVideo();
		break;
	}
	video_->setDelegate(this);

	decoder_.setDelegate(this);


	io_mgr_.buffer_func_ = std::bind([&](float percent) {
		if (player_delegate_)
			player_delegate_->onNetworkBuffer(this, percent);
	}, std::placeholders::_1);

	io_mgr_.status_func_= std::bind([&](PlayerStatus status)
	{
		NotifyPlayerStatus(status);
	},std::placeholders::_1);

}

YMediaPlayerImp::~YMediaPlayerImp()
{
	stop();
	if(audio_)
		delete audio_;
	if (video_)
		delete video_;
}


bool YMediaPlayerImp::setMedia(const char* path_file)
{
	stop();
	printf("Stop\n");

	if (!io_mgr_.SetUrl(path_file))
	{
		NotifyPlayerStatus(PlayerStatus::ErrorUrl);
		return false;
	}
	
	decoder_.SetMedia(path_file, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL);
	printf("decoder_.SetMedia\n");

	audio_->BeginPlayThread();

	video_->BeginPlayThread();


	printf("SetMediaFromFile\n");
	return true;
}

bool YMediaPlayerImp::play()
{
	audio_->Play();
	video_->Play();
	return true;
}

bool YMediaPlayerImp::pause()
{
	audio_->Pause();
	video_->Pause();
	return true;
}

bool YMediaPlayerImp::isPlaying()
{
	return audio_->IsPlaying();
}

bool YMediaPlayerImp::stop()
{
	audio_->Stop();
	decoder_.ConductAudioBlocking();
	audio_->EndPlayThread();

	video_->Stop();
	decoder_.ConductVideoBlocking();
	video_->EndPlayThread();

	io_mgr_.Conduct();
	decoder_.StopDecode();
	io_mgr_.Stop();
	return true;
}



void YMediaPlayerImp::seek(float pos)
{
	decoder_.SeekPos(media_info_.dur*pos);
	audio_->Seek(pos);
	video_->Seek(pos);
}




void YMediaPlayerImp::setOpaque(void*opa)
{
	opaque_ = opa;
}


void YMediaPlayerImp::onDecodeError(ymc::DecodeError error)
{
	switch (error)
	{
	case ymc::ERROR_FORMAT:
	{
		NotifyPlayerStatus(PlayerStatus::ErrorFormat);
	}
	break;
	case ymc::ERROR_PKG_ERROR:
	{
		NotifyPlayerStatus(PlayerStatus::ErrorDecode);
	}
	case ymc::ERROR_READ_USER_INTERRUPT:
	{
		NotifyPlayerStatus(PlayerStatus::ErrorUserInterrupt);
	}
	break;
	default:
		break;
	}
}

void YMediaPlayerImp::onMediaInfo(MediaInfo info)
{
	media_info_ = info;
	audio_->SetDuration(info.dur);
	video_->SetDuration(info.dur);

	if (player_delegate_)
		player_delegate_->onDurationChanged(this, (int)info.dur);
	printf("OnMediaInfo :Dur-%f,\n", media_info_.dur);
}

int YMediaPlayerImp::onRead(char *data, int len)
{
	return 	io_mgr_.Read(data, len);
}

int64_t YMediaPlayerImp::onSeek(int64_t offset, int whence)
{
	return 	io_mgr_.Seek(offset, whence);
}

void YMediaPlayerImp::setDelegate(YMediaPlayer::Delegate* dele)
{
	player_delegate_ = dele;
}

YMediaPlayer::Delegate* YMediaPlayerImp::getDelegate()
{
	return player_delegate_;
}

bool YMediaPlayerImp::onVideoSeek()
{
	return decoder_.JudgeBlockVideoSeek();
}

bool YMediaPlayerImp::onVideoGetData(char ** data, int *width, int *height, double *pts)
{
	VideoPackageInfo  && pkg = decoder_.PopVideoQue(video_->GetClock());
	if (pkg.error == ymc::ERROR_NO_ERROR)
	{
		*data = (char*)pkg.data;
		*width = pkg.width;
		*height = pkg.height;
		*pts = pkg.pts;
		return true;
	}
	return false;
}

bool YMediaPlayerImp::onVideoSync()
{
	while (!audio_->IsStop() || !video_->IsStop())
	{
		//printf("%f,%f \n", video_->GetClock(), audio_->GetClock());
		if (video_->GetClock() <= audio_->GetClock())
			return true;
		int delayTime = static_cast<int>((video_->GetClock() - audio_->GetClock()) * 1000);
		delayTime = delayTime > 1 ? 1 : delayTime;
		std::this_thread::sleep_for(std::chrono::milliseconds(delayTime));
	}
	return false;
}

void YMediaPlayerImp::onVideoDisplay(void *data, int width, int height)
{
	//decoder_->user_video_func_;
	if (player_delegate_)
	{
		player_delegate_->onVideoData(this, data, width, height);
	}
}

bool YMediaPlayerImp::onAudioGetData(char ** data, int *len, double *pts)
{
	AudioPackageInfo &&info = decoder_.PopAudioQue();
	if (info.error == ymc::ERROR_NO_ERROR)
	{
		*data = (char*)info.data;
		*len = info.size;
		*pts = info.pts;
		return true;
	}

	return false;
}

void YMediaPlayerImp::onAudioCurrent(int cur_pos)
{
	if(player_delegate_)
		player_delegate_->onCurrentChanged(this, cur_pos);
}

void YMediaPlayerImp::onAudioSeek()
{
	decoder_.JudgeBlockAudioSeek();
}

void YMediaPlayerImp::onAudioFreeData(char*data)
{
	if (data)
	{
		AudioPackageInfo info;
		info.data = data;
		info.error = ymc::ERROR_NO_ERROR;
		decoder_.FreeAudioPackageInfo(&info);
	}

}

void* YMediaPlayerImp::getOpaque()
{
	return opaque_;
}



void YMediaPlayerImp::NotifyPlayerStatus(PlayerStatus st)
{
	if (player_delegate_)
		player_delegate_->onStatusChanged(this,st);
}