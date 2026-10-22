#pragma once
#include "SceneClassifier.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace immersive {
// Two mutually exclusive render routes prevent duplicating the surround bus:
// BedOnly is bit-exact six-channel pass-through.
// ExtractSurroundObject conservatively transfers a small surround component
// out of SL/SR into ONE mono object. The host must render both bed and object.
enum class BedObjectRoute { BedOnly, ExtractSurroundObject };
struct BedObjectSample {
    std::array<float,6> bed{};
    float objectMono=0.0f;
    BedObjectRoute route=BedObjectRoute::BedOnly;
};
class BedObjectSplitter {
    float gain=0.0f;
public:
    void reset() { gain=0.0f; }
    BedObjectSample process(const std::array<float,6>& input,
                            const SceneDecision& decision,
                            bool spatialObjectReady,
                            float amount=1.0f) {
        BedObjectSample out;
        out.bed=input;
        const bool eligible=spatialObjectReady && decision.objectEligible &&
             decision.kind==SceneKind::MovingCandidate;
        const float target=eligible ? 0.20f*std::max(0.0f,std::min(1.0f,amount)) : 0.0f;
        // At 48kHz this is a ~10ms ramp; avoids clicks at decision boundaries.
        gain+=0.0021f*(target-gain);
        if (!spatialObjectReady || gain<0.000001f) {
            gain=spatialObjectReady?gain:0.0f;
            return out;
        }
        // Mono mid transfer; L-R difference remains in surround bed.
        const float mid=0.5f*(input[4]+input[5]);
        const float taken=gain*mid;
        out.bed[4]-=taken;
        out.bed[5]-=taken;
        out.objectMono=2.0f*taken;
        out.route=BedObjectRoute::ExtractSurroundObject;
        return out;
    }
};
} // namespace immersive
