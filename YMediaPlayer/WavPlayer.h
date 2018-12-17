#pragma once

#include <windows.h>

class WavPlayer
{
public:
	WavPlayer();
	~WavPlayer();

	bool InitPlayer(int sample_rate, int channels, int bytes_persec);

	void Reset();

	void AddBuff(char *buff,int buff_size);

	void Play();

	void Pause();
	
	
protected:


	static void CALLBACK waveOutProc(
		HWAVEOUT hWaveOut,
		UINT uMsg,
		DWORD dwInstance,
		DWORD dwParam1,
		DWORD dwParam2
	);

	WAVEHDR* allocateBlocks(int size, int count);

	void freeBlocks(WAVEHDR* blockArray);
private:
	HWAVEOUT hWaveOut_; /* device handle */

	WAVEHDR* waveBlocks_;

	volatile int waveFreeBlockCount_;

	int waveCurrentBlock_;


};



