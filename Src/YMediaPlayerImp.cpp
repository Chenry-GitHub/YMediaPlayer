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
	:status_func_(nullptr)
	, opaque_(nullptr)
	, user_video_func_(nullptr)
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
	case MODE_USER:
		video_ = new ConstomVideo();
		break;
	}
	video_->SetDataFunction(std::bind(&YMediaPlayerImp::OnVideoDataFunction, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	video_->SetSyncToAudioFunction(std::bind(&YMediaPlayerImp::OnSynchronizeVideo, this));
	video_->SetBlockSeekFunction(std::bind(&YMediaPlayerImp::OnVideoSeekFunction, this));
	video_->SetUserDisplayFunction(std::bind(&YMediaPlayerImp::OnUserDisplayFunction,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	decoder_ = new YMediaDecode();
	decoder_->SetErrorFunction(std::bind(&YMediaPlayerImp::OnDecodeError,this, std::placeholders::_1));
	decoder_->SetMediaFunction(std::bind(&YMediaPlayerImp::OnMediaInfo, this, std::placeholders::_1));
	decoder_->SetReadMemFunction(std::bind(&YMediaPlayerImp::OnReadMem, this, std::placeholders::_1, std::placeholders::_2));
	decoder_->SetSeekMemFunction(std::bind(&YMediaPlayerImp::OnSeekMem, this, std::placeholders::_1, std::placeholders::_2));

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

	if (network_)
		delete network_;
	network_ = new HttpDownload;
	network_->GetNetworkRequest()->SetUrl(path_file);
	network_->GetNetwork()->ASyncGet2(network_->GetNetworkRequest(), network_);



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

void YMediaPlayerImp::SetUserHandleVideoFunction(VideoFunc func)
{
	user_video_func_ = func;
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

bool YMediaPlayerImp::OnUserDisplayFunction(void *data, int width, int height)
{
	//decoder_->user_video_func_;
	if (user_video_func_)
	{
		user_video_func_(opaque_, data, width, height);
		return true;
	}
	return false;
}

int YMediaPlayerImp::OnReadMem(char*data, int len)
{
	int64_t download_len = network_->GetNetwork()->GetMemLen();
	while (download_len < len + cur_pos_)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(20));

		download_len = network_->GetNetwork()->GetMemLen();
	}

	int nbytes = (int64_t)std::min<int>(download_len - cur_pos_, len);
	if (nbytes <= 0) {
		return 0;
	}

	char * data_src = (char *)network_->GetNetwork()->GetMemPtr();
	memcpy_s(data, (int)nbytes, data_src + cur_pos_, (int)nbytes);

	cur_pos_ += nbytes;
	return nbytes;
}


int64_t YMediaPlayerImp::OnSeekMem(int64_t offset, int whence)
{
	//int64_t  newpos = 0;
	//switch (whence)
	//{
	//	case SEEK_SET:
	//		newpos = offset;
	//		break;
	//	case SEEK_CUR:
	//		newpos = read_fs_.tellg() + offset;
	//		break;
	//	case SEEK_END: // 此处可能有问题
	//	{
	//		std::streampos  pos = read_fs_.tellg();
	//		read_fs_.seekg(0, ios::end);
	//		int64_t totalsize = read_fs_.tellg();
	//		read_fs_.seekg(pos);
	//		newpos = totalsize+offset;
	//		if (offset > 0)
	//		{
	//			read_fs_.seekg(totalsize);
	//			return totalsize;
	//		}
	//		else
	//		{
	//			read_fs_.seekg(newpos);
	//			return newpos;
	//		}
	//		break;
	//	}
	//	case 0x10000://AVSEEK_SIZE
	//	{
	//		std::streampos  pos = read_fs_.tellg();
	//		read_fs_.seekg(0, ios::end);
	//		int64_t totalsize = read_fs_.tellg();
	//		read_fs_.seekg(pos);
	//		return totalsize;
	//	}
	//}
	//read_fs_.seekg(newpos);
	//return newpos;

	//if (whence == 0x10000)
	//{
	//	return -1;
	//}
	//printf("OnSeekMem : offset %d ,whence %d\n",offset, whence);
	//fseek(file_, offset, whence);

	//return ftell(file_);

	//int64_t new_pos = 0;


	//switch (whence) {

	//case SEEK_SET:
	//	new_pos = offset;
	//	break;
	//case SEEK_CUR:
	//	new_pos = cur_pos_ + offset;
	//	break;
	//case SEEK_END:
	//	new_pos = network_->total_ + offset;
	//	break;
	//case AVSEEK_SIZE:
	//	return -1;
	//default:
	//	return AVERROR(EINVAL);
	//}

	//cur_pos_ = FFMIN(new_pos, network_->total_);



	//return cur_pos_;
	int64_t download_len = network_->GetNetwork()->GetMemLen();
	int64_t total_len = network_->total_;
	int64_t newPos = -1;
	switch (whence) {
	case SEEK_SET: newPos = offset; break;
	case SEEK_CUR: newPos = cur_pos_ + offset; break;
	case SEEK_END: newPos = download_len + offset; break;
	case AVSEEK_SIZE: {
		// Special whence for determining filesize without any seek.
		return total_len;
	} break;
	}
	if (newPos < 0 || newPos > download_len) {
		return -1;
	}
	cur_pos_ = newPos;
	return cur_pos_;
}


void YMediaPlayerImp::NotifyPlayerStatus(PlayerStatus st)
{
	if (status_func_)
		status_func_(st);
}