// Windows-only endpoint format diagnostic. Does not open or replace the audio stream.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <cstdio>
#include <cstdint>
#include "../Source/WindowsSpeakerMap.h"

int wmain() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return 2;
    IMMDeviceEnumerator* enumeration=nullptr;
    IMMDevice* device=nullptr;
    IAudioClient* audio=nullptr;
    WAVEFORMATEX* mix=nullptr;
    int exitCode=1;
    do {
        hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
                            __uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(&enumeration));
        if (FAILED(hr)) break;
        hr=enumeration->GetDefaultAudioEndpoint(eRender,eMultimedia,&device);
        if (FAILED(hr)) break;
        hr=device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,
                            reinterpret_cast<void**>(&audio));
        if (FAILED(hr)) break;
        hr=audio->GetMixFormat(&mix);
        if (FAILED(hr) || !mix) break;
        std::uint32_t mask=0;
        if (mix->wFormatTag==WAVE_FORMAT_EXTENSIBLE &&
            mix->cbSize >= sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX)) {
            auto* ext=reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mix);
            mask=ext->dwChannelMask;
        }
        std::printf("Default audio endpoint: %u channels, %lu Hz, mask 0x%08lx\n",
            static_cast<unsigned>(mix->nChannels),
            static_cast<unsigned long>(mix->nSamplesPerSec),
            static_cast<unsigned long>(mask));
        immersive::EndpointLayout layout{mask,mix->nChannels};
        std::printf("Mix format exact 5.1: %s; exact 5.1.2: %s\n",
            immersive::hasExact51(layout)?"yes":"no",
            immersive::hasExact512(layout)?"yes":"no");
        std::puts("NOTE: shared-mode mix format alone does not prove 5.1.2 bitstream,"
                  " spatial-object access, AVR decoding, or exclusive-format support.");
        exitCode=0;
    } while(false);
    if (mix) CoTaskMemFree(mix);
    if (audio) audio->Release();
    if (device) device->Release();
    if (enumeration) enumeration->Release();
    CoUninitialize();
    if (exitCode) std::fprintf(stderr,"Endpoint inspection failed, HRESULT 0x%08lx\n",static_cast<unsigned long>(hr));
    return exitCode;
}
