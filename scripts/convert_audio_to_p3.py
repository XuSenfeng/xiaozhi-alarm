'''
Descripttion: 
Author: Xvsenfeng helloworldjiao@163.com
LastEditors: Xvsenfeng helloworldjiao@163.com
Copyright (c) 2025 by helloworldjiao@163.com, All Rights Reserved. 
'''
# convert audio files to protocol v3 stream
import librosa
import opuslib
import struct
import sys
import tqdm
import numpy as np

def encode_audio_to_opus(input_file, output_file):
    # Load audio file using librosa
    audio, sample_rate = librosa.load(input_file, sr=None, mono=False, dtype=np.float32)
    audio_resampled = librosa.resample(audio, orig_sr=sample_rate, target_sr=16000) 
    # Convert from float32 to int16  
    audio_int16 = np.clip(audio_resampled * 32767, -32768, 32767).astype(np.int16) 
    # Get left channel if stereo
    if audio_int16.ndim == 2:
        audio_int16 = audio_int16[0]
    print(sample_rate)
    # Initialize Opus encoder
    encoder = opuslib.Encoder(16000, 1, opuslib.APPLICATION_VOIP)

    # Encode audio data to Opus packets
    # Save encoded data to file
    with open(output_file, 'wb') as f:
        sample_rate = 16000 # 16000Hz
        duration = 60 # 60ms every frame
        frame_size = int(sample_rate * duration / 1000)
        for i in tqdm.tqdm(range(0, len(audio_int16) - frame_size, frame_size)):
            frame = audio_int16[i:i + frame_size]
            opus_data = encoder.encode(frame.tobytes(), frame_size=frame_size)
            # protocol format, [1u type, 1u reserved, 2u len, data]
            packet = struct.pack('>BBH', 0, 0, len(opus_data)) + opus_data
            f.write(packet)

# Example usage
if len(sys.argv) != 3:
    print('Usage: python convert.py <input_file> <output_file>')
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]
encode_audio_to_opus(input_file, output_file)
