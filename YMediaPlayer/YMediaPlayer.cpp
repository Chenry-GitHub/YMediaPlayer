#include "YMediaPlayer.h"

#include "OpenALAudio.h"
#include "OpenGLVideo.h"
#include "GDIVideo.h"


YMediaPlayer::YMediaPlayer()
	:status_func_(nullptr)
{
	audio_ = new OpenALAudio();
	audio_->SetDataFunction(std::bind(&YMediaPlayer::OnAudioDataFunction,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	audio_->SetBlockSeekFunction(std::bind(&YMediaPlayer::OnAudioSeekFunction, this));

	video_ = new GDIVideo();
	video_->SetDataFunction(std::bind(&YMediaPlayer::OnVideoDataFunction,this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	video_->SetSyncToAudioFunction(std::bind(&YMediaPlayer::synchronize_video, this));
	video_->SetBlockSeekFunction(std::bind(&YMediaPlayer::OnVideoSeekFunction, this));

	decoder_.SetErrorFunction(std::bind(&YMediaPlayer::OnDecodeError,this, std::placeholders::_1));
	decoder_.SetMediaFunction(std::bind(&YMediaPlayer::OnMediaInfo, this, std::placeholders::_1));
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

	decoder_.SetMedia(path_file, AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL);
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
	decoder_.ConductAudioBlocking();
	audio_->Stop();
	audio_->EndPlayThread();

	decoder_.ConductVideoBlocking();
	video_->Stop();
	video_->EndPlayThread();

	decoder_.StopDecode();
	return true;
}



void YMediaPlayer::Seek(float pos)
{
	decoder_.SeekPos(media_info_.dur*pos);
	audio_->Seek(pos);
	
	video_->Seek(pos);
}



void YMediaPlayer::synchronize_video()
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
	printf("OnMediaInfo :Dur-%f,\n", media_info_.dur);
}

bool YMediaPlayer::OnAudioDataFunction(char ** data, int *len, double *pts)
{
	AudioPackageInfo &&info = decoder_.PopAudioQue();
	if (info.error == ERROR_NO_ERROR)
	{
		*data = (char*)info.data;
		*len = info.size;
		*pts = info.pts;
		return true;
	}
	decoder_.FreeAudioPackageInfo(&info);
	return false;
}

bool YMediaPlayer::OnVideoDataFunction(char ** data, int *width, int *height, double *pts)
{
	VideoPackageInfo  && pkg = decoder_.PopVideoQue(video_->GetClock());
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
	return decoder_.JudgeBlockAudioSeek();
}

bool YMediaPlayer::OnVideoSeekFunction()
{
	return decoder_.JudgeBlockVideoSeek();
}

void YMediaPlayer::NotifyPlayerStatus(PlayerStatus st)
{
	if (status_func_)
		status_func_(st);
}

//
//void ShowRGBToWnd(HWND hWnd, BYTE* data, int width, int height)
//{
//	if (data == NULL)
//		return;
//
//	static BITMAPINFO *bitMapinfo = NULL;
//	static bool First = TRUE;
//
//	if (First)
//	{
//		BYTE * m_bitBuffer = new BYTE[40 + 4 * 256];//¿ª±ÙÒ»¸öÄÚ´æÇøÓò  
//
//		if (m_bitBuffer == NULL)
//		{
//			return;
//		}
//		First = FALSE;
//		memset(m_bitBuffer, 0, 40 + 4 * 256);
//		bitMapinfo = (BITMAPINFO *)m_bitBuffer;
//		bitMapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//		bitMapinfo->bmiHeader.biPlanes = 1;
//		for (int i = 0; i < 256; i++)
//		{ //ÑÕÉ«µÄÈ¡Öµ·¶Î§ (0-255)  
//			bitMapinfo->bmiColors[i].rgbBlue = bitMapinfo->bmiColors[i].rgbGreen = bitMapinfo->bmiColors[i].rgbRed = (BYTE)i;
//		}
//	}
//	bitMapinfo->bmiHeader.biHeight = -height;
//	bitMapinfo->bmiHeader.biWidth = width;
//	bitMapinfo->bmiHeader.biBitCount = 3 * 8;
//	RECT drect;
//	GetClientRect(hWnd, &drect);    //pWndÖ¸ÏòCWndÀàµÄÒ»¸öÖ¸Õë   
//	HDC hDC = GetDC(hWnd);     //HDCÊÇWindowsµÄÒ»ÖÖÊý¾ÝÀàÐÍ£¬ÊÇÉè±¸ÃèÊö¾ä±ú£»  
//	SetStretchBltMode(hDC, COLORONCOLOR);
//	StretchDIBits(hDC,
//		0,
//		0,
//		drect.right,   //ÏÔÊ¾´°¿Ú¿í¶È  
//		drect.bottom,  //ÏÔÊ¾´°¿Ú¸ß¶È  
//		0,
//		0,
//		width,      //Í¼Ïñ¿í¶È  
//		height,      //Í¼Ïñ¸ß¶È  
//		data,
//		bitMapinfo,
//		DIB_RGB_COLORS,
//		SRCCOPY
//	);
//	ReleaseDC(hWnd, hDC);
//}