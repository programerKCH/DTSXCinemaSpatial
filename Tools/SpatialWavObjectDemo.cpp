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
#include "../Source/SceneObjectMapper.h"
#include "../Source/SceneClassifier.h"
#include "../Source/Float51WavReader.h"

template <typename T> void release(T*& pointer) { if (pointer) { pointer->Release(); pointer=nullptr; } }

// Playback is deliberately opt-in: CI compiles but never runs this test.
int main(int argc, char** argv) {
    if (argc != 3 || std::strcmp(argv[1], "--wav-demo") != 0) {
        std::puts("Usage: SpatialWavObjectDemo --wav-demo input-5.1-side-float48k.wav");
        return 0;
    }
    immersive::Float51WavReader wav;
    if (!wav.open(argv[2])) { std::fputs("Invalid 48kHz 5.1-side float WAV input.\\n",stderr);return 3; }
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
        UINT64 rendered = 0;
        immersive::SceneClassifier classifier;
        bool eof = false;
        ISpatialAudioObject* moving = nullptr;
        immersive::SceneEngine analyzer;
        immersive::SceneObjectMapper mapper;
        analyzer.reset(48000.0f);
        mapper.reset(48000.0f);
        while (SUCCEEDED(hr) && !eof && rendered < 144000) {
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
                        std::array<double,6> power{{0,0,0,0,0,0}};
                        immersive::Frame scene{};
                        UINT32 used = 0;
                        for (UINT32 j=0;j<frames;++j) {
                            std::array<float,6> src{};
                            if (!wav.next(src)) { eof=true;pcm[j]=0.0f;continue; }
                            for (unsigned ch=0;ch<6;++ch)
                                power[ch] += double(src[ch])*src[ch];
                            scene=analyzer.process(src,immersive::HeightMode::Off,false);
                            // Evaluation ONLY: surround-derived mono object.
                            // Front/center/LFE bed is NOT rendered by this prototype.
                            float value=0.25f*(src[4]+src[5]);
                            if (!std::isfinite(value)) value=0.0f;
                            pcm[j]=std::max(-0.20f,std::min(0.20f,value));
                            ++used;
                        }
                        if (used>0) {
                            std::array<float,6> rms{};
                            for(unsigned ch=0;ch<6;++ch) rms[ch]=
                                static_cast<float>(std::sqrt(power[ch]/used));
                            const auto decision=classifier.analyze(rms,float(used)/48000.0f);
                            const auto position=mapper.update(scene.scene,used);
                            if (decision.objectEligible)
                                processing=moving->SetPosition(position.x,position.y,position.z);
                            else
                                processing=moving->SetPosition(0.0f,0.0f,-1.5f);
                        }
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
    std::printf("Spatial WAV object demo result: 0x%08lx; dynamic objects %u\\n",
                static_cast<unsigned long>(hr),static_cast<unsigned>(available));
    std::puts("Surround-only mono audition; not a full 5.1 player, a system-wide effect or Atmos/DTS:X encoder.");
    release(stream);
    if (ready) CloseHandle(ready);
    release(client);release(endpoint);release(enumeration);
    CoUninitialize();
    return SUCCEEDED(hr)?0:1;
}
