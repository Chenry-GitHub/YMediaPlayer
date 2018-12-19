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

	decoder_ptr_ = make_unique<YMediaDecode*>(new YMediaDecode());
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

	if (alcGetError(pCurDevice) != ALC_NO_ERROR)
		return AL_FALSE;

	return AL_TRUE;
}

bool YMediaPlayer::SetMediaFromFile(const std::string & path_file)
{
	Stop();

	path_file_ = path_file;

	(*decoder_ptr_)->SetMedia(path_file);

	std::thread th(&YMediaPlayer::PlayThread, this);
	play_thread_.swap(th);

	
	return true;
}

bool YMediaPlayer::Play()
{
	int state;
	alGetSourcei(source_id_, AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED || state == AL_INITIAL)
	{
		alSourcePlay(source_id_);
	}
	return true;
}

bool YMediaPlayer::Pause()
{
	return true;
}

bool YMediaPlayer::Stop()
{
	is_need_stop_ = true;

	alSourceStop(source_id_);
	alSourcei(source_id_, AL_BUFFER, 0);

	if (play_thread_.joinable())
	{
		play_thread_.join(); //next time !block here!
	}

	(*decoder_ptr_)->StopDecode();

	return true;
}

void YMediaPlayer::FillAudioBuff(ALuint& buf_id)
{
	PackageInfo info= (*decoder_ptr_)->PopAudioQue();
	if (info .size <= 0)
		return ;
	ALenum fmt;
	alBufferData(buf_id, AL_FORMAT_STEREO16, info.data, info.size, info.sample_rate);
	alSourceQueueBuffers(source_id_, 1, &buf_id);
	(*decoder_ptr_)->ReleasePackageInfo(&info);
}

int YMediaPlayer::PlayThread()
{
	//is_need_stop_ = false;

	//while (! is_need_stop_ )
	//{
	//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	//	
	//}
	//printf("PlayThread Safely quit!\n");
	//get frame

	is_need_stop_ = false;
	for (int i = 0; i < NUMBUFFERS; i++)
	{
		FillAudioBuff(audio_buf_[i]);
	}
	Play();

	//
	while (false == is_need_stop_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		ALint processed = 0;
		alGetSourcei(source_id_, AL_BUFFERS_PROCESSED, &processed);
		//printf("the processed is:%d\n", processed);
		while (processed > 0)
		{
			ALuint bufferID = 0;
			alSourceUnqueueBuffers(source_id_, 1, &bufferID);
			printf("bufferID:%d\n", bufferID);
			FillAudioBuff(bufferID);
			processed--;
		}
		Play();
	}


	return true;
}
