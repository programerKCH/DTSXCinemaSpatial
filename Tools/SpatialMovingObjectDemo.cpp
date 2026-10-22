// Experimental Windows Spatial Sound stream activation smoke test.
// No system-audio interception, playback, or codec encoding.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <spatialaudioclient.h>
#include <propidl.h>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>

template <typename T> void release(T*& pointer) { if (pointer) { pointer->Release(); pointer=nullptr; } }

// Playback is deliberately opt-in: CI compiles but never runs this test.
int wmain(int argc, wchar_t** argv) {
    if (argc != 2 || wcscmp(argv[1], L"--demo") != 0) {
        std::puts("Spatial moving-object prototype. Explicit --demo required to play a quiet 3-second tone.");
        return 0;
    }
    HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(co)) return 2;
    IMMDeviceEnumerator* enumeration = nullptr;
    IMMDevice* endpoint = nullptr;
    ISpatialAudioClient* client = nullptr;
    ISpatialAudioObjectRenderStream* stream = nullptr;
    HANDLE ready = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumeration));
    if (SUCCEEDED(hr)) hr = enumeration->GetDefaultAudioEndpoint(eRender,eMultimedia,&endpoint);
    if (SUCCEEDED(hr)) hr = endpoint->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER,
                                               nullptr,reinterpret_cast<void**>(&client));
    WAVEFORMATEX format = {};
    format.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;
    format.nChannels=1;
    format.nSamplesPerSec=48000;
    format.wBitsPerSample=32;
    format.nBlockAlign=4;
    format.nAvgBytesPerSec=192000;
    if (SUCCEEDED(hr)) hr=client->IsAudioObjectFormatSupported(&format);
    UINT32 available=0;
    if (SUCCEEDED(hr)) hr=client->GetMaxDynamicObjectCount(&available);
    if (SUCCEEDED(hr) && available==0) hr=HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    if (SUCCEEDED(hr)) {
        ready=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if (!ready) hr=HRESULT_FROM_WIN32(GetLastError());
    }
    if (SUCCEEDED(hr)) {
        SpatialAudioObjectRenderStreamActivationParams params={};
        params.ObjectFormat=&format;
        params.StaticObjectTypeMask=AudioObjectType_None;
        params.MinDynamicObjectCount=0;
        params.MaxDynamicObjectCount=1;
        params.Category=AudioCategory_SoundEffects;
        params.EventHandle=ready;
        params.NotifyObject=nullptr;
        PROPVARIANT blob;
        PropVariantInit(&blob);
        blob.vt=VT_BLOB;
        blob.blob.cbSize=sizeof(params);
        blob.blob.pBlobData=reinterpret_cast<BYTE*>(&params);
        hr=client->ActivateSpatialAudioStream(&blob,__uuidof(ISpatialAudioObjectRenderStream),
                 reinterpret_cast<void**>(&stream));
    }
    if (SUCCEEDED(hr)) {
        hr = stream->Start();
        const float pi = 3.14159265358979323846f;
        UINT64 rendered = 0;
        ISpatialAudioObject* moving = nullptr;
        while (SUCCEEDED(hr) && rendered < 144000) {
            if (WaitForSingleObject(ready, 100) != WAIT_OBJECT_0) {
                hr = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
                break;
            }
            UINT32 availableNow = 0, frames = 0;
            hr = stream->BeginUpdatingAudioObjects(&availableNow, &frames);
            if (FAILED(hr)) break;
            HRESULT processing = S_OK;
            if (!moving && availableNow > 0)
                processing=stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic,&moving);
            if (SUCCEEDED(processing) && moving) {
                BYTE* bytes = nullptr;
                UINT32 length = 0;
                processing = moving->GetBuffer(&bytes, &length);
                if (SUCCEEDED(processing)) {
                    if (length < frames * sizeof(float)) processing = E_FAIL;
                    else {
                        float* pcm = reinterpret_cast<float*>(bytes);
                        for (UINT32 j=0;j<frames;++j) {
                            const float t = static_cast<float>(rendered+j)/48000.0f;
                            const float fade = std::min(1.0f,std::min(t*5.0f,(3.0f-t)*5.0f));
                            pcm[j] = 0.035f * std::max(0.0f,fade) * std::sin(2.0f*pi*440.0f*t);
                        }
                        const float t = static_cast<float>(rendered)/48000.0f;
                        const float angle = (t/3.0f)*2.0f*pi;
                        processing = moving->SetPosition(1.5f*std::sin(angle),
                                                         0.4f, -1.5f*std::cos(angle));
                    }
                }
            }
            // Match every Begin with End, including when an object operation fails.
            const HRESULT ended=stream->EndUpdatingAudioObjects();
            if (FAILED(processing)) hr=processing;
            else if (FAILED(ended)) hr=ended;
            rendered += frames;
        }
        if (moving) {
            moving->SetEndOfStream(0);
            moving->Release();
        }
        stream->Stop();
    }
    std::printf("Spatial moving-object demo result: 0x%08lx; dynamic objects %u\\n",
                static_cast<unsigned long>(hr),static_cast<unsigned>(available));
    std::puts("This synthesizes its OWN quiet tone. No system-audio interception or Atmos/DTS:X encoding.");
    release(stream);
    if (ready) CloseHandle(ready);
    release(client);release(endpoint);release(enumeration);
    CoUninitialize();
    return SUCCEEDED(hr)?0:1;
}
