#include "WavAudio.h"
#if PLATFORM_IS_WIN32

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#pragma comment(lib,"winmm.lib")


#define BLOCK_SIZE 9216 //4608*2
#define BLOCK_COUNT 6

WavAudio::WavAudio()
	:is_seek_(false)
{
	initPlayer(AUDIO_OUT_SAMPLE_RATE, AUDIO_OUT_CHANNEL);
	
}

WAVEHDR* WavAudio::allocateBlocks(int size, int count)
{

	unsigned char* buffer;
	int i;
	WAVEHDR* blocks;
	DWORD totalBufferSize = (size + sizeof(WAVEHDR)) * count;
	/*
	* allocate memory for the entire set in one go
	*/
	if ((buffer = (unsigned char*)HeapAlloc(

		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		totalBufferSize
	)) == NULL)
	{
		fprintf(stderr, "Memory allocationerror\n");
		ExitProcess(1);
	}
	/*
	* and set up the pointers to each bit
	*/
	blocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * count;
	for (i = 0; i < count; i++) {

		blocks[i].dwBufferLength = size;
		blocks[i].lpData = (LPSTR)buffer;
		buffer += size;

	}
	return blocks;

}

void WavAudio::freeBlocks(WAVEHDR* blockArray)
{

	/*
	* and this is why allocateBlocks works the way it does
	*/
	HeapFree(GetProcessHeap(), 0, blockArray);

}


void WavAudio::playThread()
{

	while (false == is_stop_)
	{
		delegate_->onAudioSeek();

		if (!isPlaying())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		if (is_seek_)
		{
			WaitForPlayDone();
			is_seek_ = false;
		}

		char *data;
		int len;
		double clock;
		if (!delegate_->onAudioGetData(&data, &len, &clock))
		{
			continue;
		}
		
		WAVEHDR* current= &waveBlocks_[waveCurrentBlock_];

		/*
		* first make sure the header we're going to use is unprepared
		*/
		if (current->dwFlags & WHDR_PREPARED)
		{
			clock_ = clock_map_[waveCurrentBlock_];
			
			delegate_->onAudioCurrent(clock_);
			waveOutUnprepareHeader(hWaveOut_, current, sizeof(WAVEHDR));
		}

		memcpy(current->lpData , data, len);
		current->dwBufferLength = len;
		waveOutPrepareHeader(hWaveOut_, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut_, current, sizeof(WAVEHDR));

		clock_map_[waveCurrentBlock_] = clock;
		waveFreeBlockCount_--;
		/*
		* wait for a block to become free
		*/
		while (0 == waveFreeBlockCount_)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
			

		/*
		* point to the next block
		*/
		waveCurrentBlock_++;
		waveCurrentBlock_ %= BLOCK_COUNT;
		current = &waveBlocks_[waveCurrentBlock_];
		current->dwUser = 0;

		//free audio data
		delegate_->onAudioFreeData(data);
	}
	WaitForPlayDone();
	printf("Audio thread quit\n");
}

void WavAudio::WaitForPlayDone()
{
	/*
	* wait for all blocks to complete
	*/
	while (waveFreeBlockCount_ < BLOCK_COUNT)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	waveCurrentBlock_ = 0;
}

void WavAudio::seek(float percent)
{
	BaseAudio::seek(percent);
	is_seek_ = true;
}


WavAudio::~WavAudio()
{
	freeBlocks(waveBlocks_);
	waveOutClose(hWaveOut_);
}


bool WavAudio::initPlayer(int sample_rate,int channels)
{
	WAVEFORMATEX wfx; /* look this up in your documentation */
					  /*
					  * initialise the module variables
					  */
	waveBlocks_ = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount_ = BLOCK_COUNT;
	waveCurrentBlock_ = 0;

	/*
	* set up the WAVEFORMATEX structure.
	*/
	wfx.nSamplesPerSec = sample_rate; /* sample rate */
	wfx.wBitsPerSample = channels *8; /* sample size */
	wfx.nChannels = channels; /* channels*/
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) >> 3;
	wfx.nAvgBytesPerSec = sample_rate*channels*2;
	/*
	* try to open the default wave device. WAVE_MAPPER is
	* a constant defined in mmsystem.h, it always points to the
	* default wave device on the system (some people have 2 or
	* more sound cards).
	*/
	if (waveOutOpen(

		&hWaveOut_,
		WAVE_MAPPER,
		&wfx,
		(DWORD_PTR)waveOutProc,
		(DWORD_PTR)&waveFreeBlockCount_,
		CALLBACK_FUNCTION
	) != MMSYSERR_NOERROR)
	{

		//fprintf(stderr, "%s: unable toopen wave mapper device\n", argv[0]);
		ExitProcess(1);
		return false;
	}
	return true;
}

void CALLBACK WavAudio::waveOutProc(

	HWAVEOUT hWaveOut,
	UINT uMsg,
	DWORD dwInstance,
	DWORD dwParam1,
	DWORD dwParam2
)
{

	/*
	* pointer to free block counter
	*/
	int* freeBlockCounter = (int*)dwInstance;
	/*
	* ignore calls that occur due to openining and closing the
	* device.
	*/
	if (uMsg != WOM_DONE)

		return;

	(*freeBlockCounter)++;

}
#endif