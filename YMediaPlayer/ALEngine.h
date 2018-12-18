#ifndef __ALENGINE_H__
#define __ALENGINE_H__
#define NUMBUFFERS              (4)
#define    SERVICE_UPDATE_PERIOD    (20)

#include <al.h>
#include <alc.h>
#include  <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "YMediaPlayer2.h"
class ALEngine
{
public:
	ALEngine();
	~ALEngine();

	int OpenFile(std::string path);
	int Play(); //ֻ����ӳ�ʼ״̬��ֹͣ״̬�в���
	int PausePlay();    //ֻ�������ͣ״̬����
	int Pause();
	int Stop();
private:
	ALuint            m_source;
	std::unique_ptr<AudioDecoder> m_decoder;
	std::unique_ptr<std::thread> m_ptPlaying;

	std::atomic_bool m_bStop;

	ALuint m_buffers[NUMBUFFERS];
	ALuint m_bufferTemp;
private:
	int SoundPlayingThread();
	int SoundCallback(ALuint& bufferID);
	int InitEngine();
	int DestroyEngine();
};

#endif