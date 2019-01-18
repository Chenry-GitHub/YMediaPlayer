#include "OpenALAudio.h"



#define CLEAR_MAP(map_ )  \
for (auto iter = map_.begin(); iter != map_.end();)\
	iter = map_.erase(iter);


OpenALAudio::OpenALAudio()
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
}


OpenALAudio::~OpenALAudio()
{
	Stop();//可能退出错误，play线程被阻塞时
	alDeleteBuffers(NUMBUFFERS, audio_buf_);
	alDeleteSources(1, &source_id_);
}

int OpenALAudio::InitPlayer()
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

int OpenALAudio::UnInitPlayer()
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

void OpenALAudio::Play()
{
	BaseAudio::Play();
	int state;
	alGetSourcei(source_id_, AL_SOURCE_STATE, &state);
	if (state == AL_STOPPED || state == AL_INITIAL || state == AL_PAUSED)
	{
		alSourcePlay(source_id_);
		//TODO notify player status
	}
}

void OpenALAudio::Stop()
{
	BaseAudio::Stop();

	alSourceStop(source_id_);
	alSourcei(source_id_, AL_BUFFER, 0x00);

	CLEAR_MAP(que_map_);
}

void OpenALAudio::PlayThread()
{
	is_stop_ = false;
	//first time ,need to fill the Source
	for (int i = 0; i < NUMBUFFERS; i++)
	{
		if (!FillAudioBuff(audio_buf_[i]))
		{
			return;
		}
	}

	while (false == is_stop_)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		//if (clock_ >= 0)
		//{
		//	PlayerStatus st;
		//	st.status = PlayerStatus::Playing;
		//	NotifyPlayerStatus(st);
		//}

		//if ((int)media_info_.dur <= (int)clock_)
		//{
		//	printf("EndofAudio \n");
		//	decoder_.StopDecode();
		//	PlayerStatus st;
		//	st.status = PlayerStatus::Done;
		//	NotifyPlayerStatus(st);
		//	break;
		//}

		if (seek_func_())
		{
			alSourcei(source_id_, AL_BUFFER, 0x00);
		}



		if (!IsPlaying())
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
			clock_ = que_map_[bufferID];
			if (!FillAudioBuff(bufferID))
			{
				continue;
			}
		}
	}
}

bool OpenALAudio::FillAudioBuff(ALuint& buf)
{
		char *data;
		int len;
		double clock;
		if (!data_func_(&data,&len,&clock))
		{
			return false;
		}
		alBufferData(buf, AL_FORMAT_STEREO16, data, len, AUDIO_OUT_SAMPLE_RATE);
		alSourceQueueBuffers(source_id_, 1, &buf);
		que_map_[buf] = clock;
		return true;
}
