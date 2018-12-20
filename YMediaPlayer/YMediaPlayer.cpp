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

	Stop();
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

	path_file_ = path_file;

	decoder_.SetMedia(path_file);

	std::thread th(&YMediaPlayer::PlayThread, this);
	play_thread_.swap(th);

	return true;
}

bool YMediaPlayer::Play()
{
	int state;
	alGetSourcei(source_id_, AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED || state == AL_INITIAL || state == AL_PAUSED)
	{
		is_pause_ = false;
		alSourcePlay(source_id_);
	}
	//alSourcePlay(source_id_);
	//is_pause_ = false;
	return true;
}

bool YMediaPlayer::Pause()
{
//	alSourcePause(source_id_);
	is_pause_ = true;
	return true;
}

bool YMediaPlayer::Stop()
{
	is_need_stop_ = true;
	is_pause_ = true;

	alSourceStop(source_id_);
	alSourcei(source_id_, AL_BUFFER, 0);

	if (play_thread_.joinable())
	{
		play_thread_.join(); //next time !block here!
	}

	decoder_.StopDecode();

	return true;
}

bool YMediaPlayer::IsPause()
{
	/*int state;
	alGetSourcei(source_id_, AL_SOURCE_STATE, &state);
	if (state == AL_PAUSED)
	{
		return true;
	}
	return false;*/
	return is_pause_;
}

void YMediaPlayer::FillAudioBuff(ALuint& buf_id)
{
	PackageInfo info= decoder_.PopAudioQue();
	if (info .size <= 0)
		return ;
	ALenum fmt;
	alBufferData(buf_id, AL_FORMAT_STEREO16, info.data, info.size, info.sample_rate);
	alSourceQueueBuffers(source_id_, 1, &buf_id);
	decoder_.ReleasePackageInfo(&info);
}

int YMediaPlayer::PlayThread()
{
	is_need_stop_ = false;
	//first time ,need to fill the Source
	for (int i = 0; i < NUMBUFFERS; i++)
	{
		FillAudioBuff(audio_buf_[i]);
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
			FillAudioBuff(bufferID);
		}
	}


	return true;
}
