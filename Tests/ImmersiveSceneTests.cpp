#include "../Source/ImmersiveScene.h"
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
    assert(supported.channels == x && supported.realHeightActive);
    immersive::Frame virt{};
    for (int i=0;i<48000;++i) virt = engine.process(x, immersive::HeightMode::Virtual, false);
    assert(virt.channels[0] == x[0] && virt.channels[1] == x[1] &&
           virt.channels[2] == x[2] && virt.channels[3] == x[3]);
    assert(virt.channels[4] != x[4] && virt.channels[5] != x[5]);
    assert(virt.scene.azimuth < 0.0f);
    assert(std::isfinite(virt.scene.elevation));
}
