#include "include/PrettyMusic.h"

INITIALIZE_EASYLOGGINGPP

using namespace std;



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
	}
	setValues();
	while(cin.get() != 'x'){
		//poll data and update values
		//wait for x to stop
		if(update() != S_OK) break;
	}
	CoUninitialize();
	return 0;
} 
HRESULT deviceInit(){
	//initializing values
	
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
		if (m_wfx->wBitsPerSample == 16){
			m_format = FMT_PCM_S16;
		}
		break;

	case WAVE_FORMAT_IEEE_FLOAT:
		m_format = FMT_PCM_F32;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
		if (reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_wfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT){
			m_format = FMT_PCM_F32;
		}
		break;
	}
    LOG_IF(m_format == FMT_INVALID,WARNING) << "Invalid sample format.  Only PCM 16b integer or PCM 32b float are supported.";
    
	

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
//change type to double maybe idk probs not lmao
HRESULT update(){
	LARGE_INTEGER pcCur;
	QueryPerformanceCounter(&pcCur);
	if(m_clCapture && (((pcCur.QuadPart - m_pcPoll.QuadPart) * m_pcMult) >= QUERY_TIMEOUT)){
		BYTE* buffer;
		UINT32 nFrames;
		DWORD flags;
		UINT64 pos;
		HRESULT hr;

		while((hr=m_clCapture->GetBuffer(&buffer, &nFrames, &flags, &pos, NULL)) == S_OK){
			//temp buffers for rms and peak for processing
			float rms[MAX_CHANNELS];
			float peak[MAX_CHANNELS];
			for (int i = 0; i < MAX_CHANNELS; i++){
				rms[i] = (float)m_rms[i];
				peak[i] = (float)m_peak[i];
			}
			if(m_format == FMT_PCM_F32){
				float* s = (float*) buffer;
				//add other channel conditions later
				if(m_wfx->nChannels == 2){
					for(unsigned int i = 0; i < nFrames; i++){
						float xL = (float)*s++;
						float xR = (float)*s++;
						float sqrL = xL * xL;
						float sqrR = xR * xR;
						float absL = abs(xL);
						float absR = abs(xR);
						rms[0] = sqrL + m_kRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						rms[1] = sqrR + m_kRMS[(sqrR < rms[1])] * (rms[1] - sqrR);
						peak[0] = absL + m_kPeak[(absL < peak[0])] * (peak[0] - absL);
						peak[1] = absR + m_kPeak[(absR < peak[1])] * (peak[1] - absR);
					}
				}
				else{
					LOG(ERROR) << "Invalid number of channels";
					return E_INVALIDARG;
				}
			}
			else{
				LOG(ERROR) << "Invalid format for audio";
				return E_INVALIDARG;
			}
			for( int i = 0; i < MAX_CHANNELS; i++){
				m_rms[i] = rms[i];
				m_peak[i] = peak[i];
				//LOG(INFO) << m_rms[i] << "," << m_peak[i] << "\n";
			}
			if(m_fftSize){
				LOG(INFO) << "Made it to fft calculations";
				float* sF32 = (float*) buffer;
				INT16* sI16 = (INT16*) buffer;
				const float scalar = (float)(1.0/sqrt(m_fftSize));

				for(unsigned int iFrame = 0; iFrame < nFrames; iFrame++){
					for(unsigned int iChan = 0; iChan < m_wfx->nChannels;iChan++){
						//fills temp fft buffer with float values from correct audio format
						m_fftIn[iChan][m_fftBufW] = m_format == FMT_PCM_F32 ? *sF32++ : (float)*sI16++ * 1.0f / 0x7fff;
					}

					m_fftBufW = (m_fftBufW + 1) % m_fftSize;

					//once buffer is ready to be processed
					if(!--m_fftBufP){
						for(unsigned int iChan = 0; iChan < m_wfx->nChannels; iChan++){
							//Audio Stream has useful outputs/is not just silence
							if(!(flags & AUDCLNT_BUFFERFLAGS_SILENT)){
								// copy from the ring buffer to temp space (apparently)
								memcpy(&m_fftTmpIn[0], &(m_fftIn[iChan])[m_fftBufW], (m_fftSize - m_fftBufW) * sizeof(float));
								memcpy(&m_fftTmpIn[m_fftSize - m_fftBufW], &m_fftIn[iChan][0], m_fftBufW * sizeof(float));

								for(int iBin = 0; iBin < m_fftSize; iBin++){
									m_fftTmpIn[iBin] *= m_fftKWdw[iBin];
								}
								//Does to FFT from tmpin to tmpout
								kiss_fftr(m_fftCfg[iChan],m_fftTmpIn,m_fftTmpOut);
							}
							else{
								memset(m_fftTmpOut, 0 ,m_fftSize * sizeof(kiss_fft_cpx));
							}

							for(int iBin = 0; iBin < m_fftSize; iBin++){
								float x0 = (m_fftOut[iChan])[iBin];
								float x1 = (m_fftTmpOut[iBin].r * m_fftTmpOut[iBin].r + m_fftTmpOut[iBin].i * m_fftTmpOut[iBin].i);
								x0 = x1 + m_kFFT[(x1 < x0)] * (x0 - x1);
								m_fftOut[iChan][iBin] = x0;
							}
						}
						m_fftBufP = m_fftSize - m_fftOverlap;
					}
				}
				//integrating discrete bands from fft
				//idk why any of this happens lmao
				if(m_nBands){
					const float df = (float)m_wfx->nSamplesPerSec / m_fftSize;
					const float scalar = 2.0f / (float)m_wfx->nSamplesPerSec;
					for(unsigned int iChan = 0; iChan < m_wfx->nChannels; iChan++){
						memset(m_bandOut[iChan], 0, m_nBands * sizeof(float));
						int iBin = 0;
						int iBand = 0;
						float f0 = 0.0f;

						while(iBin <= (m_fftSize / 2) && iBand < m_nBands){
							float fLin1 = ((float) iBin + 0.5f) * df;
							float fLog1 = m_bandFreq[iBand];
							float x = (m_fftOut[iChan])[iBin];
							float& y = (m_bandOut[iChan])[iBand];
							if(fLin1 <= fLog1){
								y += (fLin1 - f0) * x * scalar;
								f0 = fLin1;
								iBin += 1;
							}
							else{
								y += (fLog1 - f0) * x * scalar;
								f0 = fLog1;
								iBand += 1;
							}
						}
					}
				}
			}
			m_clCapture->ReleaseBuffer(nFrames);
			m_pcFill = pcCur;
		}
		switch(hr){
			case AUDCLNT_E_BUFFER_ERROR:
			case AUDCLNT_E_DEVICE_INVALIDATED:
			case AUDCLNT_E_SERVICE_NOT_RUNNING:
				//deviceRelease();
				break;
		}
		switch(m_type){
			case TYPE_RMS:
				LOG(INFO) << CLAMP01((sqrt(m_rms[0]) + sqrt(m_rms[1])) * 0.5 *m_gainRMS);
				break;
			case TYPE_FFT:
			//idk yet lmao
				if(m_clCapture && m_fftSize){

				}
				break;
			case TYPE_BAND:
				if(m_clCapture && m_fftSize){
					double x;
					for(int iBand = 0; iBand < m_nBands; iBand++){
						x = (m_bandOut[0][iBand] + m_bandOut[1][iBand]) * 0.5;
						x = CLAMP01(x);
						x = max(0, 10.0 / m_sensitivity * log10(x) + 1.0);
						LOG(INFO) << "Values for band " << iBand <<": " << x;
					}
				}
				break;
		}
		
		
	}
	return S_OK;
}

void setValues(){
	m_kRMS[0] = 0.0f;
	m_kRMS[1] = 0.0f;
	m_kPeak[0] = 0.0f;
	m_kPeak[1] = 0.0f;
	for (int iChan = 0; iChan < MAX_CHANNELS; ++iChan){
			m_rms[iChan] = 0.0;
			m_peak[iChan] = 0.0;
			m_fftCfg[iChan] = NULL;
			m_fftIn[iChan] = NULL;
			m_fftOut[iChan] = NULL;
			m_bandOut[iChan] = NULL;
	}
	QueryPerformanceFrequency(&pcFreq);
	m_pcMult = 1.0 / (double)pcFreq.QuadPart;
	QueryPerformanceCounter(&m_pcPoll);
	m_envRMS[0] = 100;
	m_envRMS[1] = 300;
	m_envPeak[0] = 50;
	m_envPeak[1] = 50;
	m_envFFT[0] = 50;
	m_envFFT[1] = 50;
	m_fftOverlap = 0;
	m_nBands = 10;
	m_gainRMS = 2.5;
	m_type = TYPE_BAND;
	m_sensitivity = 10.0;
	m_fftSize = 2048;

	if(m_fftSize){
		for(int i = 0; i < m_wfx->nChannels;i++){
			m_fftCfg[i] = kiss_fftr_alloc(m_fftSize, 0, NULL, NULL);
			m_fftIn[i] = (float*)calloc(m_fftSize * sizeof(float), 1);
			m_fftOut[i] = (float*)calloc(m_fftSize * sizeof(float),1);
		}

		m_fftKWdw = (float*)calloc(m_fftSize * sizeof(float), 1);
		m_fftTmpIn = (float*)calloc(m_fftSize * sizeof(float), 1);
		m_fftTmpOut = (kiss_fft_cpx*)calloc(m_fftSize * sizeof(kiss_fft_cpx), 1);
		m_fftBufP = m_fftSize - m_fftOverlap;

		//define windows function for fft
		for(int i = 0; i < m_fftSize;i++){
			m_fftKWdw[i] = (float)(0.5 * (1.0 - cos(TWOPI * i / (m_fftSize -1))));
		}
	}

	if (m_nBands){
		m_bandFreq = (float*)malloc(m_nBands * sizeof(float));
		const double step = (log(m_freqMax / m_freqMin) / m_nBands) / log(2.0);
		m_bandFreq[0] = (float)(m_freqMin * pow(2.0,step / 2.0));

		for(int i = 1; i< m_nBands; i++){
			m_bandFreq[i] = (float)(m_bandFreq[i - 1] * pow(2.0,step));
		}

		for(int i = 0; i < m_wfx->nChannels; i++){
			m_bandOut[i] = (float*)calloc(m_nBands *  sizeof(float),1);
		}
	}

	if(m_wfx){
		const double freq = m_wfx->nSamplesPerSec;
		m_kRMS[0] = (float) exp(log10(0.01) / (freq * (double) m_envRMS[0] * 0.001));
		m_kRMS[1] = (float) exp(log10(0.01) / (freq * (double) m_envRMS[1] * 0.001));
		m_kPeak[0] = (float) exp(log10(0.01) / (freq * (double) m_envPeak[1] * 0.001));
		m_kPeak[1] = (float) exp(log10(0.01) / (freq * (double) m_envPeak[1] * 0.001));
		if (m_fftSize){
				m_kFFT[0] = (float) exp(log10(0.01) / (freq / (m_fftSize-m_fftOverlap) * (double)m_envFFT[0] * 0.001));
				m_kFFT[1] = (float) exp(log10(0.01) / (freq / (m_fftSize-m_fftOverlap) * (double)m_envFFT[1] * 0.001));
			}
	}
}