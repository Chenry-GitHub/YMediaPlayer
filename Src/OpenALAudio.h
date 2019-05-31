#pragma once
#include "BaseAudio.h"

#if USE_OPENAL

#include <al.h>
#include <alc.h>
#include <map>

#define NUMBUFFERS              4



class OpenALAudio : public BaseAudio
{
public:
	OpenALAudio();
	~OpenALAudio();

	static int InitPlayer();
	static int UnInitPlayer();

	virtual void play() override;

	virtual void pause() override;

	virtual void stop() override; 

protected:
	virtual void playThread()  override;

	bool FillAudioBuff(ALuint& buf);
private:

	ALuint		source_id_;
	ALuint audio_buf_[NUMBUFFERS];
	std::map<ALuint, double> que_map_;

	
};

#endif//USE_OPENAL