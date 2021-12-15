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
void setValues();

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

enum Type
	{
		TYPE_RMS,
		TYPE_PEAK,
		TYPE_FFT,
		TYPE_BAND,
		TYPE_FFTFREQ,
		TYPE_BANDFREQ,
		TYPE_FORMAT,
		// ... //
		NUM_TYPES
	};

IMMDeviceEnumerator*	m_enum;                     // audio endpoint enumerator
IMMDevice*				m_dev;	// audio endpoint device
WAVEFORMATEX*			m_wfx;  // audio format info
IAudioClient*			m_clAudio;  // audio client instance
IAudioCaptureClient*	m_clCapture;    // capture client instanc
Format					m_format;
Type                    m_type;
LARGE_INTEGER			m_pcFill;
LARGE_INTEGER			m_pcPoll;
double					m_rms[MAX_CHANNELS];		// current RMS levels
double					m_peak[MAX_CHANNELS];		// current peak levels
double					m_pcMult;
float					m_kRMS[2];					// RMS attack/decay filter constants
float					m_kPeak[2];					// peak attack/decay filter constants
float					m_kFFT[2];
int						m_fftSize = 0;
LARGE_INTEGER pcFreq;
int						m_envRMS[2];				// RMS attack/decay times in ms (parsed from options)
int						m_envPeak[2];				// peak attack/decay times in ms (parsed from options)
int						m_envFFT[2];
int						m_fftOverlap;				// number of samples between FFT calculations
int						m_nBands;
double					m_gainRMS;					// RMS gain (parsed from options)
kiss_fftr_cfg			m_fftCfg[MAX_CHANNELS];		// FFT states for each channel
float*					m_fftIn[MAX_CHANNELS];		// buffer for each channel's FFT input
float*					m_fftOut[MAX_CHANNELS];		// buffer for each channel's FFT output
float*					m_fftKWdw;					// window function coefficients
float*					m_fftTmpIn;					// temp FFT processing buffer
kiss_fft_cpx*			m_fftTmpOut;				// temp FFT processing buffer
int						m_fftBufW;					// write index for input ring buffers
int						m_fftBufP;					// decremental counter - process FFT at zero
float*					m_bandOut[MAX_CHANNELS];	// buffer of band values
float*					m_bandFreq;					// buffer of band max frequencies
double					m_freqMin = 20;					// min freq for band measurement
double					m_freqMax = 20000;					// max freq for band measurement
double					m_sensitivity;