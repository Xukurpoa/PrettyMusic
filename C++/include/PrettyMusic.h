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

#define EMPTY_TIMEOUT			0.500
#define DEVICE_TIMEOUT			1.500
#define QUERY_TIMEOUT			(1.0 / 60)
#define CLAMP01(x)				max(0.0, min(1.0, (x)))
#define TWOPI					(2 * 3.14159265358979323846)
#define EXIT_ON_ERROR(hres)		if (FAILED(hres)) { return hr; }
#define SAFE_RELEASE(p)			if ((p) != NULL) { (p)->Release(); (p) = NULL; }


HRESULT deviceInit();
HRESULT update();
void deviceRelease();

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
LARGE_INTEGER			m_pcPoll;
double					m_rms[MAX_CHANNELS];		// current RMS levels
double					m_peak[MAX_CHANNELS];		// current peak levels
double					m_pcMult;
float					m_kRMS[2];					// RMS attack/decay filter constants
float					m_kPeak[2];					// peak attack/decay filter constants
float					m_kFFT[2];
int						m_fftSize =0;