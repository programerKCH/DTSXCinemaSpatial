#include "../Source/SpatialHostBridge.h"
#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace immersive;
    SpatialHostBridge bridge;
    bridge.reset(48000.0f);
    bridge.configure(HeightMode::Real,HostCapabilities{true,false},1.0f);
    // Leaf-inspired rear-heavy scene: only transfer surround-derived height energy.
    for(int i=0;i<48000;++i) {
        const std::array<float,6> x={0.01f,0.01f,0.01f,0.0f,
                                     0.14f*std::sin(i*0.08f),-0.11f*std::sin(i*0.063f)};
        auto y=bridge.render(x);
        assert(y.route==Route::Discrete512);
        for(size_t c=0;c<4;++c) assert(y.internal[c]==x[c]);
        assert(std::fabs(y.internal[4]+y.internal[6]-x[4])<1.e-6f);
        assert(std::fabs(y.internal[5]+y.internal[7]-x[5])<1.e-6f);
    }
    // Final-impact scene: front/LFE-only input must never synthesize height.
    for(int i=0;i<48000;++i) {
        const std::array<float,6> x={0.06f,0.04f,0.12f,
            i%1024<100?0.75f:0.0f,0.0f,0.0f};
        auto y=bridge.render(x);
        assert(y.internal[3]==x[3] && y.internal[2]==x[2]);
        assert(y.internal[6]==0.0f && y.internal[7]==0.0f);
        assert(y.internal[4]==0.0f && y.internal[5]==0.0f);
    }
    // Unsupported real-height host never changes the PCM bed.
    bridge.configure(HeightMode::Real,HostCapabilities{false,false},1.0f);
    const std::array<float,6> x={0.5f,-0.2f,0.1f,0.2f,0.4f,-0.3f};
    for(int n=0;n<10000;++n) {
        const auto y=bridge.render(x);
        assert(y.route==Route::Bed51);
        for(size_t c=0;c<6;++c) assert(y.internal[c]==x[c]);
    }
}
