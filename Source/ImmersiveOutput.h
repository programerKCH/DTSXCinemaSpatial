#pragma once
#include "ImmersiveScene.h"
#include <array>

namespace immersive {
// Internal planar channel ordering only: L R C LFE SL SR TFL TFR.
// NOT a Windows WAVEFORMATEXTENSIBLE interleaved order and NOT Atmos metadata.
struct Planar512 {
    std::array<float,8> sample{};
    bool hasHeight = false;
};
inline Planar512 packInternal512(const Frame& f) {
    Planar512 out{};
    for (size_t i=0; i<6; ++i) out.sample[i]=f.channels[i];
    out.hasHeight=f.realHeightActive && f.realHeightAvailable;
    if (out.hasHeight) {
        out.sample[6]=f.height[0];
        out.sample[7]=f.height[1];
    }
    return out;
}
// Windows 5.1.2 output adapter must negotiate channels and map the actual
// endpoint layout; a failed negotiation must NEVER claim Real Height support.
} // namespace immersive
