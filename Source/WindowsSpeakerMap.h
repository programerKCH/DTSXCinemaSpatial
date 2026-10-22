#pragma once
#include "SpatialHostBridge.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace immersive {
// Windows Ksmedia.h speaker mask values. Intentionally portable for CI.
constexpr std::uint32_t FL=0x1, FR=0x2, FC=0x4, LFE=0x8;
constexpr std::uint32_t SL=0x200, SR=0x400, TFL=0x1000, TFR=0x4000;
constexpr std::uint32_t MASK_51_SIDE=FL|FR|FC|LFE|SL|SR;
constexpr std::uint32_t MASK_512_SIDE_FRONT=MASK_51_SIDE|TFL|TFR;
struct EndpointLayout {
    std::uint32_t mask=0;
    unsigned int channels=0;
    EndpointLayout() = default;
    EndpointLayout(std::uint32_t speakerMask, unsigned int count) : mask(speakerMask), channels(count) {}
};
inline bool hasExact512(const EndpointLayout& layout) {
    return layout.channels==8 && layout.mask==MASK_512_SIDE_FRONT;
}
inline bool hasExact51(const EndpointLayout& layout) {
    return layout.channels==6 && layout.mask==MASK_51_SIDE;
}
inline HostCapabilities capabilitiesFor(const EndpointLayout& layout) {
    HostCapabilities c{};
    c.hasDiscreteTopFrontPair=hasExact512(layout);
    // Dynamic spatial object support is not inferable from a PCM channel mask.
    c.spatialObjectsAvailable=false;
    return c;
}
// Result is an *interleaved one-frame* WASAPI sample in WAVEFORMATEXTENSIBLE
// increasing-bit mask order: FL FR FC LFE SL SR TFL TFR.
// Reject unexpected channel masks rather than quietly misrouting heights.
inline bool mapToInterleaved(const RenderSample& input, const EndpointLayout& endpoint,
                             std::array<float,8>& output) {
    output.fill(0.0f);
    const bool real=input.route==Route::Discrete512;
    if (real && !hasExact512(endpoint)) return false;
    if (!real && !hasExact51(endpoint)) return false;
    const unsigned count=real?8u:6u;
    for (unsigned i=0;i<count;++i) output[i]=input.internal[i];
    return true;
}
} // namespace immersive
