#include "WavPlayer.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#pragma comment(lib,"winmm.lib")

CRITICAL_SECTION g_waveCriticalSection;

#define BLOCK_SIZE 8192
#define BLOCK_COUNT 20

WavPlayer::WavPlayer()
{
	
	
}

WAVEHDR* WavPlayer::allocateBlocks(int size, int count)
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

void WavPlayer::freeBlocks(WAVEHDR* blockArray)
{

	/*
	* and this is why allocateBlocks works the way it does
	*/
	HeapFree(GetProcessHeap(), 0, blockArray);

}


void WavPlayer::Reset()
{

}

void WavPlayer::AddBuff(char *buff, int buff_size)
{
	WAVEHDR* current;
	int remain;
	current = &waveBlocks_[waveCurrentBlock_];
	while (buff_size > 0) {

		/*
		* first make sure the header we're going to use is unprepared
		*/
		if (current->dwFlags & WHDR_PREPARED)

			waveOutUnprepareHeader(hWaveOut_, current, sizeof(WAVEHDR));

		if (buff_size < (int)(BLOCK_SIZE - current->dwUser)) {

			memcpy(current->lpData + current->dwUser, buff, buff_size);
			current->dwUser += buff_size;
			break;

		}
		remain = BLOCK_SIZE - current->dwUser;
		memcpy(current->lpData + current->dwUser, buff, remain);
		buff_size -= remain;
		buff += remain;
		current->dwBufferLength = BLOCK_SIZE;
		waveOutPrepareHeader(hWaveOut_, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut_, current, sizeof(WAVEHDR));
		EnterCriticalSection(&g_waveCriticalSection);
		waveFreeBlockCount_--;
		LeaveCriticalSection(&g_waveCriticalSection);
		/*
		* wait for a block to become free
		*/
		while (!waveFreeBlockCount_)

			Sleep(10);

		/*
		* point to the next block
		*/
		waveCurrentBlock_++;
		waveCurrentBlock_ %= BLOCK_COUNT;
		current = &waveBlocks_[waveCurrentBlock_];
		current->dwUser = 0;

	}
}

WavPlayer::~WavPlayer()
{
	/*
	* wait for all blocks to complete
	*/
	while (waveFreeBlockCount_ < BLOCK_COUNT)
		Sleep(10);

	/*
	* unprepare any blocks that are still prepared
	*/
	for (int i = 0; i < waveFreeBlockCount_; i++)

		if (waveBlocks_[i].dwFlags &WHDR_PREPARED)

			waveOutUnprepareHeader(hWaveOut_, &waveBlocks_[i], sizeof(WAVEHDR));

	DeleteCriticalSection(&g_waveCriticalSection);
	freeBlocks(waveBlocks_);
	waveOutClose(hWaveOut_);

}


bool WavPlayer::InitPlayer(int sample_rate,int channels, int bytes_persec)
{
	WAVEFORMATEX wfx; /* look this up in your documentation */
					  /*
					  * initialise the module variables
					  */
	waveBlocks_ = allocateBlocks(BLOCK_SIZE, BLOCK_COUNT);
	waveFreeBlockCount_ = BLOCK_COUNT;
	waveCurrentBlock_ = 0;
	InitializeCriticalSection(&g_waveCriticalSection);
	/*
	* set up the WAVEFORMATEX structure.
	*/
	wfx.nSamplesPerSec = sample_rate; /* sample rate */
	wfx.wBitsPerSample = channels *8; /* sample size */
	wfx.nChannels = channels; /* channels*/
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) >> 3;
	wfx.nAvgBytesPerSec = bytes_persec;
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

	}
}

void CALLBACK WavPlayer::waveOutProc(

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

	EnterCriticalSection(&g_waveCriticalSection);
	(*freeBlockCounter)++;
	LeaveCriticalSection(&g_waveCriticalSection);

}
