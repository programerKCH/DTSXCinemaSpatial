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
#include "../Source/BedObjectSplitter.h"
#include <vector>

template <typename T> void release(T*& pointer) { if (pointer) { pointer->Release(); pointer=nullptr; } }

// Playback is deliberately opt-in: CI compiles but never runs this test.
int main(int argc, char** argv) {
    if (argc != 3 || std::strcmp(argv[1], "--wav-demo") != 0) {
        std::puts("Usage: SpatialBedObjectDemo --wav-demo input-5.1-side-float48k.wav");
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
    // A 5.1 static bed remains useful even when the endpoint has zero dynamic objects.
    if (SUCCEEDED(hr)) {
        ready=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if (!ready) hr=HRESULT_FROM_WIN32(GetLastError());
    }
    if (SUCCEEDED(hr)) {
        SpatialAudioObjectRenderStreamActivationParams params={};
        params.ObjectFormat=&format;
        params.StaticObjectTypeMask=static_cast<AudioObjectType>(
          AudioObjectType_FrontLeft|AudioObjectType_FrontRight|AudioObjectType_FrontCenter|
          AudioObjectType_LowFrequency|AudioObjectType_SideLeft|AudioObjectType_SideRight);
        params.MinDynamicObjectCount=0;
        params.MaxDynamicObjectCount=(available>0)?1u:0u;
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
        const bool started=SUCCEEDED(hr);
        UINT64 rendered = 0;
        immersive::BedObjectSplitter splitter;
        const AudioObjectType staticTypes[6]={AudioObjectType_FrontLeft,AudioObjectType_FrontRight,
           AudioObjectType_FrontCenter,AudioObjectType_LowFrequency,AudioObjectType_SideLeft,AudioObjectType_SideRight};
        ISpatialAudioObject* staticObjects[6]={nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
        immersive::SceneClassifier classifier;
        bool eof = false;
        ISpatialAudioObject* moving = nullptr;
        immersive::SceneEngine analyzer;
        immersive::SceneObjectMapper mapper;
        analyzer.reset(48000.0f);
        mapper.reset(48000.0f);
        while (SUCCEEDED(hr) && !eof) {
            if (WaitForSingleObject(ready, 2000) != WAIT_OBJECT_0) {
                hr = HRESULT_FROM_WIN32(WAIT_TIMEOUT);
                break;
            }
            UINT32 availableNow = 0, frames = 0;
            hr = stream->BeginUpdatingAudioObjects(&availableNow, &frames);
            if (FAILED(hr)) break;
            if (frames==0) { const HRESULT ended=stream->EndUpdatingAudioObjects();
                hr=FAILED(ended)?ended:HRESULT_FROM_WIN32(ERROR_INVALID_DATA); break; }
            HRESULT processing = S_OK;
            for (unsigned ch=0;ch<6 && SUCCEEDED(processing);++ch)
                if (!staticObjects[ch]) processing=stream->ActivateSpatialAudioObject(staticTypes[ch],&staticObjects[ch]);
            if (!moving && availableNow>0 && SUCCEEDED(processing)) {
                // Losing an optional dynamic voice must never mute the six-channel bed.
                ISpatialAudioObject* candidate=nullptr;
                if (SUCCEEDED(stream->ActivateSpatialAudioObject(AudioObjectType_Dynamic,&candidate))) moving=candidate;
            }
            std::vector<std::array<float,6>> bed(frames);
            std::vector<float> objectSignal(frames,0.0f);
            immersive::Frame scene{};
            std::array<double,6> powers{{0,0,0,0,0,0}};
            UINT32 used=0;
            // Pre-read a render quantum for RMS scene classification.
            std::vector<std::array<float,6>> inputs(frames);
            for(UINT32 j=0;j<frames;++j) {
                if(!wav.next(inputs[j])) { eof=true;break; }
                for(unsigned ch=0;ch<6;++ch) powers[ch]+=
                    double(inputs[j][ch])*inputs[j][ch];
                ++used;
            }
            std::array<float,6> rms{};
            if(used) for(unsigned ch=0;ch<6;++ch)
                rms[ch]=static_cast<float>(std::sqrt(powers[ch]/used));
            const auto decision=classifier.analyze(rms,
                used?float(used)/48000.0f:0.01f);
            // Dynamic object may be absent. In that case preserve the full bed.
            for(UINT32 j=0;j<used;++j) {
                scene=analyzer.process(inputs[j],immersive::HeightMode::Off,false);
                const auto part=splitter.process(inputs[j],decision,moving!=nullptr);
                bed[j]=part.bed;
                objectSignal[j]=part.objectMono;
            }
            for(unsigned ch=0;ch<6 && SUCCEEDED(processing);++ch) {
                BYTE* bytes=nullptr;UINT32 length=0;
                processing=staticObjects[ch]->GetBuffer(&bytes,&length);
                if(SUCCEEDED(processing)) {
                    if(length<frames*sizeof(float)) processing=E_FAIL;
                    else {
                        float* dest=reinterpret_cast<float*>(bytes);
                        for(UINT32 j=0;j<frames;++j)
                            dest[j]=(j<used)?bed[j][ch]:0.0f;
                    }
                }
            }
            if(SUCCEEDED(processing) && moving) {
                BYTE* bytes=nullptr;UINT32 length=0;
                processing=moving->GetBuffer(&bytes,&length);
                if(SUCCEEDED(processing)) {
                    if(length<frames*sizeof(float)) processing=E_FAIL;
                    else {
                        float* dest=reinterpret_cast<float*>(bytes);
                        for(UINT32 j=0;j<frames;++j)
                            dest[j]=(j<used)?objectSignal[j]:0.0f;
                        const auto pos=mapper.update(scene.scene,used);
                        processing=moving->SetPosition(pos.x,pos.y,pos.z);
                    }
                }
            }
            // Match every Begin with End, including when an object operation fails.
            const HRESULT ended=stream->EndUpdatingAudioObjects();
            if (FAILED(processing)) hr=processing;
            else if (FAILED(ended)) hr=ended;
            rendered += frames;
        }
        if (started) {
            const HRESULT stop=stream->Stop();
            if (SUCCEEDED(hr) && FAILED(stop)) hr=stop;
        }
        for(auto& o:staticObjects) if(o) { o->Release();o=nullptr; }
        if (moving) { moving->Release();moving=nullptr; }
    }
    std::printf("Spatial WAV object demo result: 0x%08lx; dynamic objects %u\n",
                static_cast<unsigned long>(hr),static_cast<unsigned>(available));
    std::puts("5.1 static bed plus optional extracted dynamic surround object; NOT system-wide audio interception.");
    release(stream);
    if (ready) CloseHandle(ready);
    release(client);release(endpoint);release(enumeration);
    CoUninitialize();
    return SUCCEEDED(hr)?0:1;
}
