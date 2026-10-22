// Read-only Windows WASAPI loopback availability probe.
// Explicit run only. Does not persist audio, create virtual devices,
// reroute audio, or bypass protected audio / Dolby or DTS encodings.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <cstdio>
#include <cstring>
template<class T> void done(T*& p){if(p){p->Release();p=nullptr;}}
int main(int argc, char** argv) {
    if(argc!=2 || std::strcmp(argv[1],"--probe")!=0) {
        std::puts("Usage: WasapiLoopbackProbe --probe");
        std::puts("Read-only default render endpoint inspection, max 1 second; no audio data saved.");
        return 0;
    }
    HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(init)) return 2;
    IMMDeviceEnumerator* e=nullptr;IMMDevice* d=nullptr;
    IAudioClient* a=nullptr;IAudioCaptureClient* capture=nullptr;
    WAVEFORMATEX* mix=nullptr;
    HRESULT hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
       __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&e));
    if(SUCCEEDED(hr)) hr=e->GetDefaultAudioEndpoint(eRender,eConsole,&d);
    if(SUCCEEDED(hr)) hr=d->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,
                                            reinterpret_cast<void**>(&a));
    if(SUCCEEDED(hr)) hr=a->GetMixFormat(&mix);
    if(SUCCEEDED(hr)) {
        std::printf("Endpoint mix: %u channels, %lu Hz, type=0x%x\\n",
          static_cast<unsigned>(mix->nChannels),
          static_cast<unsigned long>(mix->nSamplesPerSec),
          static_cast<unsigned>(mix->wFormatTag));
        hr=a->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,
                         10000000,0,mix,nullptr);
    }
    if(SUCCEEDED(hr)) hr=a->GetService(__uuidof(IAudioCaptureClient),
                                      reinterpret_cast<void**>(&capture));
    bool started=false;
    if(SUCCEEDED(hr)) {hr=a->Start();started=SUCCEEDED(hr);}
    unsigned long long captured=0;unsigned silent=0;
    if(SUCCEEDED(hr)) {
        for(int k=0;k<10;++k) {
            Sleep(100);
            UINT32 queued=0;
            hr=capture->GetNextPacketSize(&queued);
            while(SUCCEEDED(hr) && queued>0) {
                BYTE* data=nullptr;UINT32 frames=0;DWORD flags=0;
                hr=capture->GetBuffer(&data,&frames,&flags,nullptr,nullptr);
                if(FAILED(hr)) break;
                captured+=frames;
                if(flags&AUDCLNT_BUFFERFLAGS_SILENT)++silent;
                const HRESULT released=capture->ReleaseBuffer(frames);
                if(FAILED(released)){hr=released;break;}
                hr=capture->GetNextPacketSize(&queued);
            }
            if(FAILED(hr))break;
        }
    }
    if(started){const HRESULT stop=a->Stop();if(SUCCEEDED(hr))hr=stop;}
    std::printf("Read-only loopback: hr=0x%08lX, frames=%llu, silent packets=%u\\n",
        static_cast<unsigned long>(hr),captured,silent);
    std::puts("Capturable system mix is NOT an input path for system-wide Atmos/DTS:X processing.");
    if(mix)CoTaskMemFree(mix);
    done(capture);done(a);done(d);done(e);CoUninitialize();
    return SUCCEEDED(hr)?0:1;
}
