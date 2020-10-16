#pragma once
#define DEF_CAPTURE_MIC
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <iostream>
#include <condition_variable>
#include <thread>
#include "IULog.h"
//
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
class CBuddyChatDlg;

enum RECORDSTATE
{
	RECORDSTATE_IDLE,
	RECORDSTATE_BUSY,
	RECORDSTATE_WRITE_TO_FILE
};

#define CHANGE_TO_IDLE_RECORD  0
#define CHANGE_TO_BUSY_RECORD  1
#define WS_ERROR              -1
class videoRecord {
public:
	~videoRecord();
	videoRecord();
	void runInLoop();
	void init();
	bool recordOneTime();
	void SaveWaveData(BYTE* CaptureBuffer, size_t BufferSize, const WAVEFORMATEX* WaveFormat);
	bool WriteWaveFile(HANDLE FileHandle, const BYTE* Buffer, const size_t BufferSize, const WAVEFORMATEX* WaveFormat);

	void chatClosed(int userId , CBuddyChatDlg* BuddyChatDlg){
		std::lock_guard<std::mutex> guard(m_mutexProtectRecord);
		if(stillPlaying && m_UserId == userId && m_CBuddyChatDlg == BuddyChatDlg){
			m_CBuddyChatDlg = NULL;
			stillPlaying = false;
			m_UserId = -1;
			return;
		}
	}
	int changeState(int userId , CBuddyChatDlg* BuddyChatDlg)//仅其他线程调用
	{
		
        std::lock_guard<std::mutex> guard(m_mutexProtectRecord);
		//LOG_INFO("userId %d , BuddyChatDlg: %x m_UserId : %d",userId,BuddyChatDlg,m_UserId);
		if(stillPlaying)
		{
			if(m_UserId == userId)
			{
				m_CBuddyChatDlg = NULL;
				stillPlaying    = false;
				m_UserId        = -1;
				return CHANGE_TO_IDLE_RECORD;
			}else{
				return WS_ERROR;
			}
			
		}
		else
		{
			
			{
				std::unique_lock<std::mutex> guard(m_mutexProtectState);
				if(m_state != RECORDSTATE_IDLE)
					return WS_ERROR;
			}
		
			m_CBuddyChatDlg = BuddyChatDlg;
			m_UserId        = userId;
			stillPlaying    = true;
			m_cvVideo.notify_one();
			return CHANGE_TO_BUSY_RECORD;
		}

		
		
	}

	bool  queryRadioRes()
	{
		hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
		if (hr != S_OK) 
		{
			m_haveRadioRes = false;
		}
		else if (m_haveRadioRes == false) {
			if(m_videoThread.get() != NULL)
				reset();
			else
				init();
			m_haveRadioRes = true;
		}
		return m_haveRadioRes;
	};
private:
	void reset();
	void  quit(){ 
		m_stop = true;
		LOG_INFO("m_stop = true");
		{
			std::lock_guard<std::mutex> guard(m_mutexProtectRecord);
			m_cvVideo.notify_one();
		}
	};
public:
	std::mutex                      m_mutexProtectRecord;  
	std::mutex                      m_mutexProtectState;
	std::condition_variable 		m_cvVideo;
	
private:
	std::unique_ptr<std::thread>    m_videoThread;
	BOOL   							stillPlaying;//需要线程安全么
	CBuddyChatDlg*				    m_CBuddyChatDlg = NULL;//zeo todo 可以用weak ptr
	BOOL							m_stop;
	BOOL							m_haveRadioRes;
	BOOL							m_ini;
	int								m_UserId;
	RECORDSTATE						m_state;
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
