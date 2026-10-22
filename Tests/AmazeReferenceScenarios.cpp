#include "../Source/SpatialHostBridge.h"
#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace immersive;
    SpatialHostBridge bridge;
    const HostCapabilities noTop{false,false};
    const HostCapabilities top{true,false};
    bridge.reset(48000.f);
    bridge.configure(HeightMode::Virtual,noTop,1.f);
    // Sustained immersive rain: protect center/LFE and keep the virtual effect bounded.
    for(int i=0;i<48000;++i) {
        const float phase=float(i)*0.079f;
        const std::array<float,6> x{0.02f,0.02f,0.12f,0.0f,
                                   0.16f*std::sin(phase),0.15f*std::cos(phase)};
        auto y=bridge.render(x);
        for(size_t c=0;c<4;++c) assert(y.internal[c]==x[c]);
        assert(std::fabs(y.internal[4]) <= std::fabs(x[4]) + 0.05f);
        assert(std::fabs(y.internal[5]) <= std::fabs(x[5]) + 0.05f);
        assert(y.internal[6]==0 && y.internal[7]==0);
    }
    // Sudden thunder burst: never change front/center/LFE, even as the scene is loud.
    for(int i=0;i<2000;++i) {
        const std::array<float,6> x{0.4f,0.4f,0.6f,0.85f,
                                   0.6f*std::sin(float(i)*0.11f),
                                   0.6f*std::cos(float(i)*0.11f)};
        auto y=bridge.render(x);
        for(size_t c=0;c<4;++c) assert(y.internal[c]==x[c]);
        for(float v:y.internal) assert(std::isfinite(v));
    }
    // Real Height is separated, not duplicated. Surround + Height == original.
    bridge.reset(48000.f);
    bridge.configure(HeightMode::Real,top,1.f);
    for(int i=0;i<48000;++i) {
        const std::array<float,6> x{0.01f,0.03f,0.05f,0.0f,
                                   0.11f*std::sin(float(i)*0.2f),
                                   0.12f*std::cos(float(i)*0.19f)};
        auto y=bridge.render(x);
        assert(y.route==Route::Discrete512);
        for(size_t c=0;c<4;++c) assert(y.internal[c]==x[c]);
        assert(std::fabs(y.internal[4]+y.internal[6]-x[4])<1e-6f);
        assert(std::fabs(y.internal[5]+y.internal[7]-x[5])<1e-6f);
    }
}
