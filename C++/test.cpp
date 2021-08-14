#pragma comment( lib, "Advapi32.lib" )
#pragma comment( lib, "Rpcrt4.lib" )
#pragma comment( lib, "Ole32.lib" )
#pragma comment( lib, "Winhttp.lib" )
#pragma comment( lib, "Crypt32.lib" ) 
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <Windows.h> 
#include <AudioClient.h>
#include <AudioPolicy.h>
#include <MMDeviceApi.h>
#include <cassert>
#include <Combaseapi.h>
#include "include/easylogging++.h"
#include "kiss_fft130/kiss_fftr.h"


INITIALIZE_EASYLOGGINGPP

using namespace std;

#define EMPTY_TIMEOUT			0.500
#define DEVICE_TIMEOUT			1.500
#define QUERY_TIMEOUT			(1.0 / 60)
#define CLAMP01(x)				max(0.0, min(1.0, (x)))
#define TWOPI					(2 * 3.14159265358979323846)
#define EXIT_ON_ERROR(hres)		if (FAILED(hres)) { return hr; }
#define SAFE_RELEASE(p)			if ((p) != NULL) { (p)->Release(); (p) = NULL; }


HRESULT deviceInit();
void update();

enum Channel{
		CHANNEL_FL,
		CHANNEL_FR,
		CHANNEL_C,
		CHANNEL_LFE,
		CHANNEL_BL,
		CHANNEL_BR,
		CHANNEL_SL,
		CHANNEL_SR,
		MAX_CHANNELS,
		CHANNEL_SUM = MAX_CHANNELS
	};
enum Format
	{
		FMT_INVALID,
		FMT_PCM_S16,
		FMT_PCM_F32,
		// ... //
		NUM_FORMATS
	};

IMMDeviceEnumerator*	m_enum;
IMMDevice*				m_dev;	
WAVEFORMATEX*			m_wfx;
IAudioClient*			m_clAudio;
IAudioCaptureClient*	m_clCapture;
Format					m_format;
LARGE_INTEGER			m_pcFill;


int main(int argc,char* argv[]) {
    START_EASYLOGGINGPP(argc, argv);
    /*vector<string> msg {"Hello", "C++", "World", "from", "VS Code", "and the C++ extension!"};

    for (const string& word : msg)
    {
        cout << word << " ";
    }
    cout << endl;*/
    LOG(INFO) << "My first info log using default logger";

	HRESULT res = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	LOG_IF(res != S_OK,ERROR) << "Could not cointialize the com model";
	res = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_enum);
    if ( res == S_OK){
		deviceInit();
		return 0;
	}
	while(cin.get() != 'x'){
		//poll data and update values
		//wait for x to stop
		update();
	}
	CoUninitialize();
} 
HRESULT deviceInit(){
    //assert(m_enum);
    HRESULT hr;
    hr = m_enum->GetDefaultAudioEndpoint(eRender,eConsole,&m_dev);
    EXIT_ON_ERROR(hr);
    hr = m_dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_clAudio);
	if (hr != S_OK){
        LOG(ERROR) << "Failed to create audio client.";
	}

	EXIT_ON_ERROR(hr);

    hr = m_clAudio->GetMixFormat(&m_wfx);
	EXIT_ON_ERROR(hr);

    //Find audio format
    //Probably not needed but just for safety sake
    switch (m_wfx->wFormatTag){
	case WAVE_FORMAT_PCM:
		if (m_wfx->wBitsPerSample == 16)
		{
			m_format = FMT_PCM_S16;
		}
		break;

	case WAVE_FORMAT_IEEE_FLOAT:
		m_format = FMT_PCM_F32;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
		if (reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_wfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			m_format = FMT_PCM_F32;
		}
		break;
	}
    LOG_IF(m_format == FMT_INVALID,WARNING) << "Invalid sample format.  Only PCM 16b integer or PCM 32b float are supported.";
    //FILL IN WITH FFT INITIALIZATION
    REFERENCE_TIME hnsRequestedDuration = 10000000;

    //Audio Client
    hr = m_clAudio->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,hnsRequestedDuration,0,m_wfx,NULL);
    if(hr != S_OK){
        //Fix for bug from rainmeter idk if needed
        m_wfx->nChannels = 2;
		m_wfx->nBlockAlign = (2 * m_wfx->wBitsPerSample) / 8;
		m_wfx->nAvgBytesPerSec = m_wfx->nSamplesPerSec * m_wfx->nBlockAlign;

		hr = m_clAudio->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,hnsRequestedDuration, 0, m_wfx, NULL);
        LOG_IF(hr != S_OK,WARNING) << "Failed to initialize audio client.";
    }
    EXIT_ON_ERROR(hr);

    //Audio capture client
    hr = m_clAudio->GetService(__uuidof(IAudioCaptureClient),(void**)&m_clCapture);
    LOG_IF(hr != S_OK,WARNING) << "Failed to create audio capture client.";
    EXIT_ON_ERROR(hr);

    hr = m_clAudio->Start();
    LOG_IF(hr != S_OK,WARNING) << "Failed to start the stream.";
    EXIT_ON_ERROR(hr);

    QueryPerformanceCounter(&m_pcFill);

    return S_OK;
}

void update(){
	LARGE_INTEGER pcCur;
	QueryPerformanceCounter(&pcCur);
	if(m_clCapture /*&& (pcCur.QuadPart - m_pcPoll.QuadPart) * m_pcMult) >= QUERY_TIMEOUT*/){
		BYTE* buffer;
		UINT32 nFrames;
		DWORD flags;
		UINT64 pos;
		
	}
}