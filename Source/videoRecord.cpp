#include"stdafx.h"
#include"videoRecord.h"
#include "BuddyChatDlg.h"
#include "path.h"
#include<winerror.h>
#include <functional>

#ifndef EXIT_ON_ERROR(hres)

#define EXIT_ON_ERROR(hres) if(FAILED(hres)){ exit(1); }

#endif // !EXIT_ON_ERROR(hres)

const BYTE WaveHeader[] =
{
	'R',   'I',   'F',   'F',  0x00,  0x00,  0x00,  0x00, 'W',   'A',   'V',   'E',   'f',   'm',   't',   ' ', 0x00, 0x00, 0x00, 0x00
};

//  Static wave DATA tag.
const BYTE WaveData[] = { 'd', 'a', 't', 'a' };

videoRecord::videoRecord(){
	m_haveRadioRes = false;
	  stillPlaying = false;
	  m_ini        = false;
	  m_stop       = true;
};
videoRecord::~videoRecord(){
	quit();
    if (m_videoThread.get() )
        m_videoThread->join();
	LOG_INFO("~videoRecord thread ptr: %x",m_videoThread.get());

	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)

		CoUninitialize();
};
void videoRecord::reset() {


/*
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		LOG_INFO("Unable to initialize COM in thread: %x\n", hr);
		return ;
	}*/


	LOG_INFO("GetDefaultAudioEndpoint : %d",hr);
	EXIT_ON_ERROR(hr)
	LOG_INFO("GetDefaultAudioEndpoint : %d",hr);

		// 创建一个管理对象，通过它可以获取到你需要的一切数据
		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);\
		LOG_INFO("pDevice->Activate   %d", hr);
	EXIT_ON_ERROR(hr)
		LOG_INFO("pDevice->Activate   %d", hr);

		hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr)
	nFrameSize = (pwfx->wBitsPerSample / 8) * pwfx->nChannels;
	cout << "nFrameSize           : " << nFrameSize << " Bytes" << endl
		<< "hnsRequestedDuration : " << hnsRequestedDuration
		<< " REFERENCE_TIME time units. 即(" << hnsRequestedDuration / 10000 << "ms)" << endl;
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
	EXIT_ON_ERROR(hr)
		
	hr = pAudioClient->GetStreamLatency(&hnsStreamLatency);
	EXIT_ON_ERROR(hr)
	cout << "GetStreamLatency     : " << hnsStreamLatency
		<< " REFERENCE_TIME time units. 即(" << hnsStreamLatency / 10000 << "ms)" << endl;
	hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, &hnsMinimumDevicePeriod);
	EXIT_ON_ERROR(hr)
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)
		
	LOG_INFO("GetBufferSize        :  %u",bufferFrameCount);
	
	 hAudioSamplesReadyEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
	if (hAudioSamplesReadyEvent == NULL)
	{
		LOG_INFO("Unable to create samples ready event: %d.\n", GetLastError());
		exit(1);
		//goto Exit;
	}
	hr = pAudioClient->SetEventHandle(hAudioSamplesReadyEvent);
	if (FAILED(hr))
	{
		LOG_INFO("Unable to set ready event: %x.\n", hr);
		return ;
	}

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)


	m_stop = false;
	m_haveRadioRes = true;
	m_ini = true;

	if (m_videoThread.get() == NULL)
        m_videoThread.reset(new std::thread(std::bind(&videoRecord::runInLoop, this)));
	
	
};

void videoRecord::init() {
	{
		std::unique_lock<std::mutex> guard(m_mutexProtectState);
		m_state = RECORDSTATE_IDLE;
	}


/*
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		LOG_INFO("Unable to initialize COM in thread: %x\n", hr);
		return ;
	}*/
	hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
		NULL,
		CLSCTX_ALL,
		IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	LOG_INFO("CoCreateInstance : %d",hr);
	EXIT_ON_ERROR(hr)
	LOG_INFO("CoCreateInstance : %d",hr);
	hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
	if (hr != S_OK) {
		m_haveRadioRes = false;
		return;
	}
	LOG_INFO("GetDefaultAudioEndpoint : %d",hr);
	EXIT_ON_ERROR(hr)
	LOG_INFO("GetDefaultAudioEndpoint : %d",hr);

		// 创建一个管理对象，通过它可以获取到你需要的一切数据
		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);\
		LOG_INFO("pDevice->Activate   %d", hr);
	EXIT_ON_ERROR(hr)
		LOG_INFO("pDevice->Activate   %d", hr);

		hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr)
	nFrameSize = (pwfx->wBitsPerSample / 8) * pwfx->nChannels;
	cout << "nFrameSize           : " << nFrameSize << " Bytes" << endl
		<< "hnsRequestedDuration : " << hnsRequestedDuration
		<< " REFERENCE_TIME time units. 即(" << hnsRequestedDuration / 10000 << "ms)" << endl;
	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
		hnsRequestedDuration,
		0,
		pwfx,
		NULL);
	EXIT_ON_ERROR(hr)
		
	hr = pAudioClient->GetStreamLatency(&hnsStreamLatency);
	EXIT_ON_ERROR(hr)
	cout << "GetStreamLatency     : " << hnsStreamLatency
		<< " REFERENCE_TIME time units. 即(" << hnsStreamLatency / 10000 << "ms)" << endl;
	hr = pAudioClient->GetDevicePeriod(&hnsDefaultDevicePeriod, &hnsMinimumDevicePeriod);
	EXIT_ON_ERROR(hr)
	hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)
		
	LOG_INFO("GetBufferSize        :  %u",bufferFrameCount);
	
	 hAudioSamplesReadyEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
	if (hAudioSamplesReadyEvent == NULL)
	{
		LOG_INFO("Unable to create samples ready event: %d.\n", GetLastError());
		exit(1);
		//goto Exit;
	}
	hr = pAudioClient->SetEventHandle(hAudioSamplesReadyEvent);
	if (FAILED(hr))
	{
		LOG_INFO("Unable to set ready event: %x.\n", hr);
		return ;
	}

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)


	m_stop = false;
	stillPlaying = false;
	m_haveRadioRes = true;
	m_ini = true;
    if (m_videoThread.get() == NULL)
        m_videoThread.reset(new std::thread(std::bind(&videoRecord::runInLoop, this)));
	
	
};
void videoRecord::runInLoop(){
	LOG_INFO(" videoRecord::runInLoop begin!");
	while(!m_stop){
		{
			std::unique_lock<std::mutex> guard(m_mutexProtectRecord);
			while (m_stop == FALSE && 
				  (stillPlaying == FALSE ||m_haveRadioRes == FALSE)    )
			{
				LOG_INFO("video thread sleep");
				m_cvVideo.wait(guard);
			}
			LOG_INFO("video thread run");
		}
		if(!m_stop)
			recordOneTime();
	}
	LOG_INFO(" m_stop videoRecord::runInLoop exit!");
}
bool videoRecord::recordOneTime() {
	hr = pAudioClient->Start();  // Start recording.
	EXIT_ON_ERROR(hr)


	int  nCnt = 0;

	size_t nCaptureBufferSize = 8*8 * 1024 * 1024;
	size_t nCurrentCaptureIndex = 0;

	BYTE* pbyCaptureBuffer = new (std::nothrow) BYTE[nCaptureBufferSize];

	HANDLE waitArray[3];
	waitArray[0] = hAudioSamplesReadyEvent;



		// Each loop fills about half of the shared buffer.
		while (!m_stop && stillPlaying)
		{
			{
				std::unique_lock<std::mutex> guard(m_mutexProtectState);
				m_state = RECORDSTATE_BUSY;
			}

			DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, 20);
			switch (waitResult)
			{
			case WAIT_OBJECT_0 + 0:     // _AudioSamplesReadyEvent
				hr = pCaptureClient->GetNextPacketSize(&packetLength);
				EXIT_ON_ERROR(hr)

					//LOG_INFO("%06d # _AudioSamplesReadyEvent packetLength:%06u \n", nCnt, packetLength);

				while (packetLength != 0)
				{
					// Get the available data in the shared buffer.
					// 锁定缓冲区，获取数据
					hr = pCaptureClient->GetBuffer(&pData,
						&numFramesAvailable,
						&flags, NULL, NULL);
					EXIT_ON_ERROR(hr)


						nCnt++;

					// test flags
					//////////////////////////////////////////////////////////////////////////
					if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
					{
						LOG_INFO("AUDCLNT_BUFFERFLAGS_SILENT \n");
					}

					if (flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)
					{
						LOG_INFO("%06d # AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY \n", nCnt);
					}
					//////////////////////////////////////////////////////////////////////////

					UINT32 framesToCopy = min(numFramesAvailable, static_cast<UINT32>((nCaptureBufferSize - nCurrentCaptureIndex) / nFrameSize));
					if (framesToCopy != 0)
					{
						//
						//  The flags on capture tell us information about the data.
						//
						//  We only really care about the silent flag since we want to put frames of silence into the buffer
						//  when we receive silence.  We rely on the fact that a logical bit 0 is silence for both float and int formats.
						//
						if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
						{
							//
							//  Fill 0s from the capture buffer to the output buffer.
							//
							ZeroMemory(&pbyCaptureBuffer[nCurrentCaptureIndex], framesToCopy * nFrameSize);
						}
						else
						{
							//
							//  Copy data from the audio engine buffer to the output buffer.
							//
							CopyMemory(&pbyCaptureBuffer[nCurrentCaptureIndex], pData, framesToCopy * nFrameSize);
						}
						//
						//  Bump the capture buffer pointer.
						//
						nCurrentCaptureIndex += framesToCopy * nFrameSize;
					}

					hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
					EXIT_ON_ERROR(hr)

						hr = pCaptureClient->GetNextPacketSize(&packetLength);
					EXIT_ON_ERROR(hr)

						// test GetCurrentPadding
						//////////////////////////////////////////////////////////////////////////
						/*
						This method retrieves a padding value that indicates the amount of
						valid, unread data that the endpoint buffer currently contains.
						返回buffer中合法的未读取的数据大小。
						The padding value is expressed as a number of audio frames.
						The size in bytes of an audio frame equals
						the number of channels in the stream multiplied by the sample size per channel.
						For example, the frame size is four bytes for a stereo (2-channel) stream with 16-bit samples.
						The padding value的单位是audio frame。
						一个audio frame的大小等于 通道数 * 每个通道的sample大小。
						For a shared-mode capture stream, the padding value reported by GetCurrentPadding
						specifies the number of frames of capture data
						that are available in the next packet in the endpoint buffer.
						*/
						UINT32 ui32NumPaddingFrames;
					hr = pAudioClient->GetCurrentPadding(&ui32NumPaddingFrames);
					EXIT_ON_ERROR(hr)
						if (0 != ui32NumPaddingFrames)
						{
							printf("GetCurrentPadding : %6u\n", ui32NumPaddingFrames);
						}
					//////////////////////////////////////////////////////////////////////////


					

				} // end of 'while (packetLength != 0)'

				break;
				default:
					LOG_INFO("no service  break the loop");
					m_haveRadioRes = false;
					goto ENDDING;
			} // end of 'switch (waitResult)'

		} 
		  //
		  //  We've now captured our wave data.  Now write it out in a wave file.
		  //
	ENDDING:
	{
		std::unique_lock<std::mutex> guard(m_mutexProtectState);
		m_state = RECORDSTATE_WRITE_TO_FILE;
	}
	SaveWaveData(pbyCaptureBuffer, nCurrentCaptureIndex, pwfx);

	//LOG_INFO("Audio Capture Done. nCnt:　%d",nCnt);
	
	hr = pAudioClient->Stop();  // Stop recording.
	{
		std::unique_lock<std::mutex> guard(m_mutexProtectState);
		m_state = RECORDSTATE_IDLE;
	}

	EXIT_ON_ERROR(hr)

	return true;
};

bool videoRecord::WriteWaveFile(HANDLE FileHandle, const BYTE* Buffer, const size_t BufferSize, const WAVEFORMATEX* WaveFormat)
{
	DWORD waveFileSize = sizeof(WAVEHEADER) + sizeof(WAVEFORMATEX) + WaveFormat->cbSize + sizeof(WaveData) + sizeof(DWORD) + static_cast<DWORD>(BufferSize);
	BYTE* waveFileData = new (std::nothrow) BYTE[waveFileSize];
	BYTE* waveFilePointer = waveFileData;
	WAVEHEADER* waveHeader = reinterpret_cast<WAVEHEADER*>(waveFileData);
	
	if (waveFileData == NULL)
	{
		LOG_INFO("Unable to allocate %d bytes to hold output wave data\n", waveFileSize);
		return false;
	}

	//
	//  Copy in the wave header - we'll fix up the lengths later.
	//
	CopyMemory(waveFilePointer, WaveHeader, sizeof(WaveHeader));
	waveFilePointer += sizeof(WaveHeader);

	//
	//  Update the sizes in the header.
	//
	waveHeader->dwSize = waveFileSize - (2 * sizeof(DWORD));
	waveHeader->dwFmtSize = sizeof(WAVEFORMATEX) + WaveFormat->cbSize;

	//
	//  Next copy in the WaveFormatex structure.
	//
	CopyMemory(waveFilePointer, WaveFormat, sizeof(WAVEFORMATEX) + WaveFormat->cbSize);
	waveFilePointer += sizeof(WAVEFORMATEX) + WaveFormat->cbSize;


	//
	//  Then the data header.
	//
	CopyMemory(waveFilePointer, WaveData, sizeof(WaveData));
	waveFilePointer += sizeof(WaveData);
	*(reinterpret_cast<DWORD*>(waveFilePointer)) = static_cast<DWORD>(BufferSize);
	waveFilePointer += sizeof(DWORD);

	//
	//  And finally copy in the audio data.
	//
	CopyMemory(waveFilePointer, Buffer, BufferSize);

	//
	//  Last but not least, write the data to the file.
	//
	DWORD bytesWritten;
	if (!WriteFile(FileHandle, waveFileData, waveFileSize, &bytesWritten, NULL))
	{
		LOG_INFO("Unable to write wave file: %d\n", GetLastError());
		delete[]waveFileData;
		return false;
	}

	if (bytesWritten != waveFileSize)
	{
		LOG_INFO("Failed to write entire wave file\n");
		delete[]waveFileData;
		return false;
	}
	delete[]waveFileData;
	return true;
}

void videoRecord::SaveWaveData(BYTE* CaptureBuffer, size_t BufferSize, const WAVEFORMATEX* WaveFormat)
{
	HRESULT hr = NOERROR;

	SYSTEMTIME st;
	GetLocalTime(&st);
	char waveFileName[_MAX_PATH] = { 0 };
	sprintf_s(waveFileName, ".\\WAS_%04d-%02d-%02d_%02d_%02d_%02d_%02d.wav",
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	HANDLE waveHandle = CreateFileA((LPCSTR)waveFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	if (waveHandle != INVALID_HANDLE_VALUE)
	{
		if (WriteWaveFile(waveHandle, CaptureBuffer, BufferSize, WaveFormat))
		{
			LOG_INFO("Successfully wrote WAVE data to %s\n", waveFileName);
		}
		else
		{
			LOG_INFO("Unable to write wave file\n");
		}
		CloseHandle(waveHandle);
	}
	else
	{
		LOG_INFO("Unable to open output WAV file %s: %d\n", waveFileName, GetLastError());
	}
	string aa(waveFileName, strlen(waveFileName) + 1);
	tstring strFileName = Hootina::CPath::GetAppPath();	// 播放来消息提示音
	//zeo todo
	for(int i = 0; waveFileName[i] !='\0';i++)
		strFileName.push_back(waveFileName[i]);
	LOG_INFO(strFileName.c_str());
	::sndPlaySound(strFileName.c_str(), SND_ASYNC);
	if(m_CBuddyChatDlg)
		m_CBuddyChatDlg->m_voiceFileName = strFileName;
}


