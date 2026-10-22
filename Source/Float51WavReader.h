#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>

namespace immersive {
// Narrow, safe input for offline experiments: RIFF/WAVE IEEE float32, 6 channels,
// 48 kHz, WAVE_FORMAT_EXTENSIBLE 5.1 SIDE mask, little-endian host.
// No decoder for TrueHD/Atmos and no system-wide audio capture.
class Float51WavReader {
    std::ifstream file;
    std::uint64_t remaining = 0;
    bool valid = false;
    static bool readU16(std::istream& s, std::uint16_t& v) {
        unsigned char b[2];if (!s.read(reinterpret_cast<char*>(b),2)) return false;
        v=static_cast<std::uint16_t>(b[0]|(std::uint16_t(b[1])<<8));return true;
    }
    static bool readU32(std::istream& s, std::uint32_t& v) {
        unsigned char b[4];if (!s.read(reinterpret_cast<char*>(b),4)) return false;
        v=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);return true;
    }
public:
    bool open(const std::string& path) {
        valid=false;remaining=0;file.close();file.clear();file.open(path,std::ios::binary);
        if (!file) return false;
        char tag[4]; std::uint32_t riffSize=0;
        if (!file.read(tag,4)||std::memcmp(tag,"RIFF",4)||!readU32(file,riffSize)||
            !file.read(tag,4)||std::memcmp(tag,"WAVE",4)) return false;
        bool fmtOk=false;
        while (file.read(tag,4)) {
            std::uint32_t size=0;if(!readU32(file,size))return false;
            const auto start=file.tellg();
            if (std::memcmp(tag,"fmt ",4)==0 && size>=40) {
                std::uint16_t format=0, channels=0, alignment=0, bits=0, extra=0;
                std::uint32_t rate=0, bytesPerSec=0, channelMask=0;
                if(!readU16(file,format)||!readU16(file,channels)||!readU32(file,rate)||
                   !readU32(file,bytesPerSec)||!readU16(file,alignment)||
                   !readU16(file,bits)||!readU16(file,extra))return false;
                std::uint16_t validBits=0; std::uint32_t subFormat=0;
                if(!readU16(file,validBits)||!readU32(file,channelMask)||
                   !readU32(file,subFormat))return false;
                // The whole IEEE_FLOAT subtype GUID is mandatory, not just Data1=3.
                const unsigned char expectedGuidTail[12]={
                    0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71
                };
                unsigned char guidTail[12]={};
                if(!file.read(reinterpret_cast<char*>(guidTail),12)) return false;
                fmtOk=(format==0xfffe && channels==6 && rate==48000 &&
                       bytesPerSec==48000u*24u &&
                       alignment==24 && bits==32 && extra>=22 &&
                       validBits==32 && channelMask==0x60f && subFormat==3 &&
                       std::memcmp(guidTail,expectedGuidTail,12)==0);
            } else if(std::memcmp(tag,"data",4)==0) {
                if(!fmtOk || size%24!=0) return false;
                remaining=size;valid=true;return true;
            }
            file.seekg(start+std::streamoff(size+(size&1u)));
            if(!file) return false;
        }
        return false;
    }
    bool next(std::array<float,6>& out) {
        if(!valid||remaining<24) return false;
        unsigned char bytes[24];
        if(!file.read(reinterpret_cast<char*>(bytes),24)) { valid=false;return false; }
        for(unsigned c=0;c<6;++c) {
            std::uint32_t word=0;
            const unsigned i=c*4;
            word=std::uint32_t(bytes[i])|(std::uint32_t(bytes[i+1])<<8)|
                 (std::uint32_t(bytes[i+2])<<16)|(std::uint32_t(bytes[i+3])<<24);
            std::memcpy(&out[c],&word,4);
            // Reject corrupted/non-finite input before it reaches spatial DSP.
            if ((word & 0x7f800000u) == 0x7f800000u) {valid=false;return false;}
        }
        remaining-=24;return true;
    }
    std::uint64_t framesRemaining() const {return remaining/24;}
};
} // namespace immersive
