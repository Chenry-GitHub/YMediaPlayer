#pragma once
#include "BaseAudio.h"

#include <al.h>
#include <alc.h>

#include <map>

#define NUMBUFFERS              4
#define  AUDIO_OUT_SAMPLE_RATE 44100
#define AUDIO_OUT_CHANNEL 2


class OpenALAudio : public BaseAudio
{
public:
	OpenALAudio();
	~OpenALAudio();

	static int InitPlayer();
	static int UnInitPlayer();

	virtual void Play() override;

	virtual void Stop() override; 

protected:
	virtual void PlayThread()  override;

	bool FillAudioBuff(ALuint& buf);
private:

	ALuint		source_id_;
	ALuint audio_buf_[NUMBUFFERS];
	std::map<ALuint, double> que_map_;
};

