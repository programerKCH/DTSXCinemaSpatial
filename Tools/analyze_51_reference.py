#!/usr/bin/env python3
"""Analyze externally supplied 5.1 reference audio. Does not redistribute the source."""
import argparse
import json
import subprocess
import numpy as np

def analyze(path, step=5):
    probe = subprocess.run(
        ["ffprobe", "-v", "error", "-select_streams", "a:0",
         "-show_entries", "stream=sample_rate,channels,channel_layout,codec_name",
         "-of", "json", path], capture_output=True, text=True, check=True)
    info = json.loads(probe.stdout)["streams"][0]
    if int(info["channels"]) != 6 or info.get("channel_layout") not in ("5.1(side)", "5.1"):
        raise ValueError("Reference must be an explicit 5.1 source; found " + str(info))
    # FFmpeg's 5.1 order is FL FR FC LFE SL SR after channel-layout conversion.
    sr = int(info["sample_rate"])
    run = subprocess.run(
        ["ffmpeg", "-v", "error", "-i", path, "-vn",
         "-af", "aformat=channel_layouts=5.1(side)",
         "-f", "f32le", "-acodec", "pcm_f32le", "-"],
        stdout=subprocess.PIPE, check=True)
    y = np.frombuffer(run.stdout, dtype="<f4")
    if len(y) % 6: raise ValueError("Corrupted decoded channel samples")
    y = y.reshape((-1,6))
    output = {"audio": info, "duration_seconds": len(y)/sr, "windows": []}
    for start in range(0, len(y), step*sr):
        block = y[start:min(start+step*sr,len(y))]
        power = (block.astype(np.float64)**2).mean(axis=0)
        total = max(float(power.sum()),1e-18)
        output["windows"].append({
            "start_seconds":start/sr,
            "end_seconds":(start+len(block))/sr,
            "rms":[round(float(np.sqrt(v)),6) for v in power],
            "surround_energy_fraction":round(float((power[4]+power[5])/total),4),
            "center_energy_fraction":round(float(power[2]/total),4),
            "lfe_energy_fraction":round(float(power[3]/total),4),
        })
    return output

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("reference_video")
    parser.add_argument("--seconds",type=int,default=5)
    args=parser.parse_args()
    if args.seconds<1: parser.error("--seconds must be positive")
    print(json.dumps(analyze(args.reference_video,args.seconds),indent=2))
