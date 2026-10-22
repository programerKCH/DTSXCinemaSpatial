#include "../Source/ImmersiveOutput.h"
#include <array>
#include <cassert>
#include <cmath>
int main() {
    immersive::SceneEngine engine;
    engine.reset(48000);
    const std::array<float,6> x{{0.4f,0.1f,0.15f,0.25f,0.12f,-0.16f}};
    auto off = engine.process(x, immersive::HeightMode::Off, false);
    assert(off.channels == x);
    auto unsupported = engine.process(x, immersive::HeightMode::Real, false);
    assert(unsupported.channels == x && !unsupported.realHeightActive);
    auto supported = engine.process(x, immersive::HeightMode::Real, true);
    assert(supported.realHeightActive);
    const auto packed = immersive::packInternal512(supported);
    assert(packed.hasHeight && packed.sample[6] == supported.height[0] &&
           packed.sample[7] == supported.height[1]);
    const auto noHeight = immersive::packInternal512(unsupported);
    assert(!noHeight.hasHeight && noHeight.sample[6] == 0.0f && noHeight.sample[7] == 0.0f);
    assert(supported.channels[0] == x[0] && supported.channels[1] == x[1]);
    assert(supported.channels[2] == x[2] && supported.channels[3] == x[3]);
    assert(std::fabs((supported.channels[4] + supported.height[0]) - x[4]) < 1e-6f);
    assert(std::fabs((supported.channels[5] + supported.height[1]) - x[5]) < 1e-6f);
    immersive::Frame height{};
    for (int i=0;i<48000;++i)
        height = engine.process(x, immersive::HeightMode::Real, true);
    assert(std::fabs(height.height[0]) > 1e-8f || std::fabs(height.height[1]) > 1e-8f ||
           (std::fabs(height.channels[4]-x[4]) < 1e-6f && std::fabs(height.channels[5]-x[5]) < 1e-6f));
    assert(std::fabs(height.channels[4] + height.height[0] - x[4]) < 1e-6f);
    assert(std::fabs(height.channels[5] + height.height[1] - x[5]) < 1e-6f);
    immersive::Frame virt{};
    for (int i=0;i<48000;++i) virt = engine.process(x, immersive::HeightMode::Virtual, false);
    assert(virt.channels[0] == x[0] && virt.channels[1] == x[1] &&
           virt.channels[2] == x[2] && virt.channels[3] == x[3]);
    assert(virt.channels[4] != x[4] && virt.channels[5] != x[5]);
    // v5 VST path: virtual widening must not change the rear-channel sum.
    assert(std::fabs((virt.channels[4]+virt.channels[5])-(x[4]+x[5]))<1e-6f);
    engine.reset(48000);
    const std::array<float,6> nearPeak{{0.2f,0.1f,0.0f,0.0f,0.95f,-0.95f}};
    immersive::Frame protectedFrame{};
    for(int i=0;i<48000;++i) protectedFrame=engine.process(
        nearPeak,immersive::HeightMode::Virtual,false);
    assert(std::fabs(protectedFrame.channels[4])<=0.980001f);
    assert(std::fabs(protectedFrame.channels[5])<=0.980001f);
    assert(std::fabs(protectedFrame.channels[4]+protectedFrame.channels[5])<1e-5f);
    // Rear fly-by must be distinguishable even with silent front channels.
    engine.reset(48000);
    immersive::Frame rearLeft{}, rearRight{};
    const std::array<float,6> leftRear{{0,0,0,0,0.35f,0.02f}};
    const std::array<float,6> rightRear{{0,0,0,0,0.02f,0.35f}};
    for(int i=0;i<48000;++i)
        rearLeft=engine.process(leftRear,immersive::HeightMode::Virtual,false);
    for(int i=0;i<48000;++i)
        rearRight=engine.process(rightRear,immersive::HeightMode::Virtual,false);
    assert(rearLeft.scene.azimuth < -10.0f);
    assert(rearRight.scene.azimuth > 10.0f);
    assert(rearRight.channels[2] == 0.0f && rearRight.channels[3] == 0.0f);
    assert(virt.scene.azimuth < 0.0f);
    assert(std::isfinite(virt.scene.elevation));
}
