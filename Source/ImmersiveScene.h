#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Experimental source-independent spatial scene analysis. No codec encoding.
namespace immersive {
enum class HeightMode { Off, Virtual, Real };
struct ObjectEstimate {
    float azimuth = 0.0f;     // degrees: left negative
    float elevation = 0.0f;   // heuristic, NOT authored metadata
    float confidence = 0.0f;
    float motion = 0.0f;
};
struct Frame {
    std::array<float, 6> channels{}; // L,R,C,LFE,SL,SR
    ObjectEstimate scene{};
    bool realHeightAvailable = false;
    bool realHeightActive = false;
};
class SceneEngine {
    float sampleRate = 48000.0f;
    float leftEnergy = 0, rightEnergy = 0, centerEnergy = 0;
    float surroundEnergy = 0, previousAzimuth = 0, smoothAzimuth = 0;
    float movement = 0, virtualEnvelope = 0;
public:
    void reset(float rate) {
        sampleRate = std::max(rate, 8000.0f);
        leftEnergy = rightEnergy = centerEnergy = surroundEnergy = 0;
        previousAzimuth = smoothAzimuth = movement = virtualEnvelope = 0;
    }
    Frame process(const std::array<float, 6>& input, HeightMode mode,
                  bool spatialRendererHasHeight, float amount = 1.0f) {
        Frame result;
        result.channels = input; // bit-for-bit fallback for Off and unsupported Real
        result.realHeightAvailable = spatialRendererHasHeight;
        const auto smooth = [this](float hz) { return 1.0f - std::exp(-6.2831853f * hz / sampleRate); };
        const float energyStep = smooth(18.0f), directionStep = smooth(7.0f);
        leftEnergy += energyStep * (input[0]*input[0] - leftEnergy);
        rightEnergy += energyStep * (input[1]*input[1] - rightEnergy);
        centerEnergy += energyStep * (input[2]*input[2] - centerEnergy);
        const float ambient = 0.5f * (input[4]*input[4] + input[5]*input[5]);
        surroundEnergy += energyStep * (ambient - surroundEnergy);
        const float total = leftEnergy + rightEnergy + centerEnergy + surroundEnergy + 1.0e-8f;
        const float targetAz = 70.0f * (rightEnergy - leftEnergy) / total;
        smoothAzimuth += directionStep * (targetAz - smoothAzimuth);
        movement += smooth(5.0f) * (std::abs(smoothAzimuth - previousAzimuth) - movement);
        previousAzimuth = smoothAzimuth;
        result.scene.azimuth = smoothAzimuth;
        result.scene.confidence = std::clamp((leftEnergy + rightEnergy) / total * (1.0f - centerEnergy / total), 0.0f, 1.0f);
        result.scene.motion = std::clamp(movement * 0.1f, 0.0f, 1.0f);
        const float diffuse = std::clamp(surroundEnergy / total, 0.0f, 1.0f);
        result.scene.elevation = 55.0f * diffuse; // proxy only; no true source location
        if (mode == HeightMode::Virtual) {
            // Virtual cue: subtle, source-derived, diffuse surround ONLY. No dialogue/LFE changes.
            const float target = std::clamp(amount, 0.0f, 1.0f) * diffuse;
            virtualEnvelope += smooth(3.0f) * (target - virtualEnvelope);
            const float width = std::clamp(0.10f * virtualEnvelope, 0.0f, 0.10f);
            result.channels[4] = input[4] + width * (input[4] - input[5]);
            result.channels[5] = input[5] + width * (input[5] - input[4]);
        } else if (mode == HeightMode::Real && spatialRendererHasHeight) {
            // No fake height in PCM: the separate spatial output adapter must consume scene metadata.
            result.realHeightActive = true;
        }
        return result;
    }
};
} // namespace immersive
