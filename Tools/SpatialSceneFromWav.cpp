#include "../Source/Float51WavReader.h"
#include "../Source/SceneObjectMapper.h"
#include "../Source/SceneClassifier.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if(argc!=3) {
        std::cerr<<"Usage: SpatialSceneFromWav input-5.1-float.wav positions.csv\n";
        return 2;
    }
    immersive::Float51WavReader reader;
    if(!reader.open(argv[1])) {
        std::cerr<<"Input must be 48 kHz six-channel 32-bit float WAVE_FORMAT_EXTENSIBLE "
                    "with 5.1(side) channel mask.\n";
        return 3;
    }
    std::ofstream csv(argv[2],std::ios::trunc);
    if(!csv) return 4;
    csv<<"time_seconds,azimuth_deg,elevation_heuristic_deg,confidence,x,y,z,scene_kind,object_eligible,virtual_wet\n";
    immersive::SceneEngine scene;
    immersive::SceneObjectMapper mapper;
    immersive::SceneClassifier classifier;
    std::array<double,6> blockPower{{0,0,0,0,0,0}};
    scene.reset(48000);mapper.reset(48000);
    std::array<float,6> sample{};
    std::uint64_t count=0;
    while(reader.next(sample)) {
        const auto frame=scene.process(sample,immersive::HeightMode::Off,false);
        ++count;
        for(unsigned ch=0;ch<6;++ch) blockPower[ch]+=double(sample[ch])*sample[ch];
        if(count%480==0) {
            std::array<float,6> rms{};
            for(unsigned ch=0;ch<6;++ch){rms[ch]=float(std::sqrt(blockPower[ch]/480.0));blockPower[ch]=0;}
            const auto decision=classifier.analyze(rms,0.01f);
            const auto xyz=mapper.update(frame.scene,480);
            csv<<std::fixed<<std::setprecision(6)<<double(count)/48000.0<<","
               <<frame.scene.azimuth<<","<<frame.scene.elevation<<","
               <<frame.scene.confidence<<","<<xyz.x<<","<<xyz.y<<","<<xyz.z<<","
               <<static_cast<int>(decision.kind)<<","<<(decision.objectEligible?1:0)<<","
               <<decision.virtualWet<<"\n";
        }
    }
    if(!csv) return 5;
    std::cout<<"Exported "<<count<<" PCM frames of heuristic positions; "
             <<"not Atmos object metadata.\n";
    return count ? 0 : 6;
}
