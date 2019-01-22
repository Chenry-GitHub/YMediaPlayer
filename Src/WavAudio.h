#pragma once

#include "BaseAudio.h"

#include <windows.h>
#include <atomic>
#include <map>


class WavAudio:public BaseAudio
{
public:
	WavAudio();
	~WavAudio();

	bool InitPlayer(int sample_rate, int channels, int bytes_persec);
	
	virtual void Seek(float percent) override;

	virtual void Stop() override;

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

	virtual void PlayThread() override;

	void WaitForPlayDone();
private:
	HWAVEOUT hWaveOut_; /* device handle */

	WAVEHDR* waveBlocks_;

	std::atomic_int waveFreeBlockCount_;

	int waveCurrentBlock_;

	std::map<int, double> clock_map_;

	std::atomic_bool is_seek_;
};



