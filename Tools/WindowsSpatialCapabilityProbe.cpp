// Read-only Windows Spatial Sound capability probe.
// Does not capture system audio, create objects, or alter the default endpoint.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <spatialaudioclient.h>
#include <cstdio>

int wmain() {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(init)) return 2;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* endpoint = nullptr;
    ISpatialAudioClient* spatial = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumerator));
    if (SUCCEEDED(hr)) hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &endpoint);
    if (SUCCEEDED(hr)) hr = endpoint->Activate(__uuidof(ISpatialAudioClient), CLSCTX_ALL,
                                               nullptr, reinterpret_cast<void**>(&spatial));
    if (SUCCEEDED(hr)) {
        UINT32 maxObjects = 0;
        const HRESULT countResult = spatial->GetMaxDynamicObjectCount(&maxObjects);
        if (SUCCEEDED(countResult)) {
            std::printf("Spatial Audio API accessible. Max dynamic objects: %u\n",
                        static_cast<unsigned>(maxObjects));
        } else {
            std::printf("Spatial API exists; object count query failed: 0x%08lx\n",
                        static_cast<unsigned long>(countResult));
        }
        std::puts("This does NOT verify an Atmos/DTS:X license, selected provider,");
        std::puts("object-stream activation, AVR decoding or Equalizer APO capture.");
    } else {
        std::printf("Spatial API unavailable on default multimedia endpoint: 0x%08lx\n",
                    static_cast<unsigned long>(hr));
    }
    if (spatial) spatial->Release();
    if (endpoint) endpoint->Release();
    if (enumerator) enumerator->Release();
    CoUninitialize();
    return SUCCEEDED(hr) ? 0 : 1;
}
