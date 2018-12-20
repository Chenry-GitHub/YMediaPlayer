#include "YMediaPlayer.h"


YMediaPlayer::YMediaPlayer()
{
	alGenSources(1, &source_id_);
	ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };
	ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };
	ALfloat ListenerPos[] = { 0.0, 0, 0 };
	ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
	// first 3 elements are "at", second 3 are "up"
	ALfloat ListenerOri[] = { 0.0, 0.0, -1.0,  0.0, 1.0, 0.0 };
	alSourcef(source_id_, AL_PITCH, 1.0);
	alSourcef(source_id_, AL_GAIN, 1.0);
	alSourcefv(source_id_, AL_POSITION, SourcePos);
	alSourcefv(source_id_, AL_VELOCITY, SourceVel);
	alSourcef(source_id_, AL_REFERENCE_DISTANCE, 50.0f);
	alSourcei(source_id_, AL_LOOPING, AL_FALSE);
	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	alListener3f(AL_POSITION, 0, 0, 0);

	alGenBuffers(NUMBUFFERS, audio_buf_);

	Pause();
}

YMediaPlayer::~YMediaPlayer()
{
	Stop();
	alDeleteBuffers(NUMBUFFERS, audio_buf_);
	alDeleteSources(1, &source_id_);
}

int YMediaPlayer::InitPlayer()
{
	ALCdevice* pDevice;
	ALCcontext* pContext;

	pDevice = alcOpenDevice(NULL);
	pContext = alcCreateContext(pDevice, NULL);
	alcMakeContextCurrent(pContext);

	if (alcGetError(pDevice) != ALC_NO_ERROR)
		return AL_FALSE;

	return AL_TRUE;
}

int YMediaPlayer::UnInitPlayer()
{
	ALCcontext* pCurContext;
	ALCdevice* pCurDevice;

	pCurContext = alcGetCurrentContext();
	pCurDevice = alcGetContextsDevice(pCurContext);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pCurContext);
	alcCloseDevice(pCurDevice);

	return AL_TRUE;
}

bool YMediaPlayer::SetMediaFromFile(const std::string & path_file)
{
	Stop();
	printf("Stop\n");

	path_file_ = path_file;

	decoder_.SetMedia(path_file);
	printf("decoder_.SetMedia\n");

	play_thread_ = std::move(std::thread(&YMediaPlayer::PlayThread, this));

	printf("SetMediaFromFile\n");
	return true;
}

bool YMediaPlayer::Play()
{
	is_pause_ = false;

	int state;
	alGetSourcei(source_id_, AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED || state == AL_INITIAL || state == AL_PAUSED)
	{
		alSourcePlay(source_id_);
	}
	return true;
}

bool YMediaPlayer::Pause()
{
	is_need_stop_ = false;
	is_pause_ = true;
	return true;
}

bool YMediaPlayer::Stop()
{
	if (play_thread_.joinable())
	{
		is_need_stop_ = true;
		is_pause_ = true;

		alSourceStop(source_id_);
		alSourcei(source_id_, AL_BUFFER, 0);
		decoder_.StopDecode();

		play_thread_.join(); //next time !block here!
	}
	return true;
}

bool YMediaPlayer::IsPause()
{
	return is_pause_;
}

YMediaPlayerError YMediaPlayer::FillAudioBuff(ALuint& buf)
{
	PackageInfo info= decoder_.PopAudioQue();
	if (info .size <= 0 || info.error  != ERROR_NO_ERROR)
		return info.error;
	ALenum fmt;
	alBufferData(buf, AL_FORMAT_STEREO16, info.data, info.size, info.sample_rate);
	alSourceQueueBuffers(source_id_, 1, &buf);
	decoder_.ReleasePackageInfo(&info);

	return ERROR_NO_ERROR;
}

int YMediaPlayer::PlayThread()
{
	//first time ,need to fill the Source
	for (int i = 0; i < NUMBUFFERS; i++)
	{
		if (YMediaPlayerError::ERROR_NO_ERROR != FillAudioBuff(audio_buf_[i]))
		{
			is_need_stop_ = true;
			break;
		}
	}

	while ( false == is_need_stop_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (IsPause())
		{
			continue;
		}
		else
		{
			Play();
		}

		ALint processed = 0;
		alGetSourcei(source_id_, AL_BUFFERS_PROCESSED, &processed);
		while (processed--)
		{
			ALuint bufferID = 0;
			alSourceUnqueueBuffers(source_id_, 1, &bufferID);
			printf("bufferID:%d\n", bufferID);
			if (YMediaPlayerError::ERROR_NO_ERROR != FillAudioBuff(bufferID))
			{
				break;
			}
		}
	}


	//while (true)
	//{
	//	//GetValue

	//	//Play Value
	//}


	//printf("PlayThread Quit:%d\n", this_thread::get_id());
	return true;
}
