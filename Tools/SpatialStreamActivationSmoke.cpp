// Experimental Windows Spatial Sound stream activation smoke test.
// No system-audio interception, playback, or codec encoding.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <spatialaudioclient.h>
#include <propidl.h>
#include <cstdio>

template <typename T> void release(T*& pointer) { if (pointer) { pointer->Release(); pointer=nullptr; } }

int wmain() {
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
    std::printf("Spatial stream activation (without playback): 0x%08lx; dynamic objects %u\n",
                static_cast<unsigned long>(hr),static_cast<unsigned>(available));
    std::puts("This is a host capability smoke test, NOT an Atmos/DTS:X renderer or system-wide effect.");
    release(stream);
    if (ready) CloseHandle(ready);
    release(client);release(endpoint);release(enumeration);
    CoUninitialize();
    return SUCCEEDED(hr)?0:1;
}
