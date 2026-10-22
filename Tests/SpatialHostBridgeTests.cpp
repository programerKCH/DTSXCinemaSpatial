#include "../Source/SpatialHostBridge.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <random>

int main() {
    immersive::SpatialHostBridge host;
    host.reset(48000.0f);
    const std::array<float,6> x = {0.28f,-0.14f,0.12f,0.20f,0.24f,-0.17f};
    immersive::HostCapabilities stereoTop{true,false};
    immersive::HostCapabilities noTop{false,true}; // objects != negotiated 5.1.2

    // OFF and unsupported REAL preserve each original PCM sample exactly.
    host.configure(immersive::HeightMode::Off, stereoTop, 1.0f);
    auto off = host.render(x);
    assert(off.route == immersive::Route::Bed51);
    for (size_t i=0;i<6;++i) assert(off.internal[i]==x[i]);
    assert(off.internal[6]==0.0f && off.internal[7]==0.0f);

    host.configure(immersive::HeightMode::Real, noTop, 1.0f);
    auto fallback=host.render(x);
    assert(fallback.route==immersive::Route::Bed51);
    for (size_t i=0;i<6;++i) assert(fallback.internal[i]==x[i]);
    assert(fallback.internal[6]==0.0f && fallback.internal[7]==0.0f);

    host.configure(immersive::HeightMode::Virtual, noTop, 1.0f);
    auto virtualHeight=host.render(x);
    assert(virtualHeight.route==immersive::Route::Virtual51);
    for (size_t i=0;i<4;++i) assert(virtualHeight.internal[i]==x[i]);
    assert(virtualHeight.internal[6]==0.0f && virtualHeight.internal[7]==0.0f);

    host.configure(immersive::HeightMode::Real, stereoTop, 1.0f);
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> signal(-0.3f,0.3f);
    float maxAbs=0.0f;
    for(int n=0;n<96000;++n) {
        const std::array<float,6> v={signal(rng),signal(rng),signal(rng),
                                     signal(rng),signal(rng),signal(rng)};
        auto result=host.render(v);
        assert(result.route==immersive::Route::Discrete512);
        for(float y:result.internal) {
            assert(std::isfinite(y));
            maxAbs=std::max(maxAbs,std::fabs(y));
        }
        for (size_t i=0;i<4;++i) assert(result.internal[i]==v[i]);
        assert(std::fabs((result.internal[4]+result.internal[6])-v[4])<1e-6f);
        assert(std::fabs((result.internal[5]+result.internal[7])-v[5])<1e-6f);
        assert(std::isfinite(result.estimate.azimuth));
        assert(std::isfinite(result.estimate.elevation));
    }
    assert(maxAbs<0.5f);

    // Zero strength must leave the original 5.1 bed exactly as-is, even in 5.1.2.
    host.configure(immersive::HeightMode::Real, stereoTop, 0.0f);
    auto zero=host.render(x);
    for(size_t i=0;i<6;++i) assert(zero.internal[i]==x[i]);
    assert(zero.internal[6]==0.0f && zero.internal[7]==0.0f);
}
