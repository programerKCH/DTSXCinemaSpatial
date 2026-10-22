#pragma once
#include "ImmersiveOutput.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace immersive {
// Minimal per-sample integration surface for a native host, not an Atmos encoder.
// The host negotiates its physical output layout first, then calls configure().
struct HostCapabilities {
    bool hasDiscreteTopFrontPair = false;
    bool spatialObjectsAvailable = false;
    HostCapabilities() = default;
    HostCapabilities(bool heights, bool objects) : hasDiscreteTopFrontPair(heights), spatialObjectsAvailable(objects) {}
};
enum class Route { Bed51, Virtual51, Discrete512 };
struct RenderSample {
    std::array<float,8> internal{}; // L R C LFE SL SR TFL TFR (NOT WAV/Windows order)
    Route route = Route::Bed51;
    ObjectEstimate estimate{};
};
class SpatialHostBridge {
    SceneEngine engine;
    HeightMode requested = HeightMode::Off;
    HostCapabilities caps{};
    float wet = 1.0f;
public:
    void reset(float sampleRate) { engine.reset(sampleRate); }
    void configure(HeightMode mode, HostCapabilities available, float amount) {
        requested = mode;
        caps = available;
        wet = bounded(amount, 0.0f, 1.0f);
    }
    Route selectedRoute() const {
        if (requested == HeightMode::Virtual) return Route::Virtual51;
        if (requested == HeightMode::Real && caps.hasDiscreteTopFrontPair)
            return Route::Discrete512;
        return Route::Bed51; // explicit no-height fallback
    }
    RenderSample render(const std::array<float,6>& original) {
        const Route route = selectedRoute();
        const HeightMode actualMode =
            route == Route::Virtual51 ? HeightMode::Virtual :
            route == Route::Discrete512 ? HeightMode::Real : HeightMode::Off;
        const auto frame = engine.process(original, actualMode,
                                           route == Route::Discrete512, wet);
        const auto packed = packInternal512(frame);
        RenderSample out{};
        out.internal = packed.sample;
        out.route = route;
        out.estimate = frame.scene;
        return out;
    }
};
// A host must implement channel-mask mapping and real WASAPI/spatial endpoint
// negotiation; channel order from RenderSample is deliberately internal.
// Native Dolby Atmos/DTS:X bitstreams are never fabricated by this bridge.
} // namespace immersive
