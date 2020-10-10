#pragma once
#define DEF_CAPTURE_MIC
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <iostream>
#include <condition_variable>
#include <thread>
#include "IULog.h"

using namespace std;
#define REFTIMES_PER_SEC       (10000000)
#define REFTIMES_PER_MILLISEC  (10000)

#define EXIT_ON_ERROR(hres)  \
	if (FAILED(hres)) {  }

#define SAFE_RELEASE(punk)  \
	if ((punk) != NULL)  \
				{ (punk)->Release(); (punk) = NULL; }


#define MoveMemory RtlMoveMemory
#define CopyMemory RtlCopyMemory
#define FillMemory RtlFillMemory
#define ZeroMemory RtlZeroMemory

#define min(a,b)            (((a) < (b)) ? (a) : (b))
struct WAVEHEADER
{
	DWORD   dwRiff;                     // "RIFF"
	DWORD   dwSize;                     // Size
	DWORD   dwWave;                     // "WAVE"
	DWORD   dwFmt;                      // "fmt "
	DWORD   dwFmtSize;                  // Wave Format Size
};

//  Static RIFF header, we'll append the format to it.


class videoRecord {
public:
	~videoRecord();
	videoRecord();
	void runInLoop();
	void init();
	bool recordOneTime();
	void SaveWaveData(BYTE* CaptureBuffer, size_t BufferSize, const WAVEFORMATEX* WaveFormat);
	bool WriteWaveFile(HANDLE FileHandle, const BYTE* Buffer, const size_t BufferSize, const WAVEFORMATEX* WaveFormat);
	int changeState(int userId)//仅其他线程调用
	{
		
        std::lock_guard<std::mutex> guard(m_mutexProtectRecord);
		LOG_INFO("stillPlaying %d ,threadid:%d",stillPlaying,std::this_thread::get_id());

		if(stillPlaying && m_UserId != userId)
		  	return -1;
		
		if(stillPlaying && m_UserId == userId){
			stillPlaying = false;
			m_UserId = -1;
			return 0;
		}
		m_UserId = userId;
		stillPlaying = true;
		m_cvVideo.notify_one();
		return 1;
	}
	void  quit(){ m_stop = true;};

public:
	std::mutex                      m_mutexProtectRecord;  
	
	std::condition_variable 		m_cvVideo;
	bool   							stillPlaying;
private:
	std::unique_ptr<std::thread>    m_videoThread;
	BOOL							m_stop;
	int								m_UserId;

	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID   IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID   IID_IAudioClient = __uuidof(IAudioClient);
	const IID   IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
	HRESULT hr;

	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;
	WAVEFORMATEX* pwfx = NULL;

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	UINT32         bufferFrameCount;
	UINT32         numFramesAvailable;

	BYTE* pData;
	UINT32         packetLength = 0;
	DWORD          flags;
	INT32          nFrameSize;
	REFERENCE_TIME hnsStreamLatency;
	REFERENCE_TIME hnsDefaultDevicePeriod;
	REFERENCE_TIME hnsMinimumDevicePeriod;
	HANDLE hAudioSamplesReadyEvent;
	
};
