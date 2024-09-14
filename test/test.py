import pylame
import numpy as np
import soundfile as sf
import math

input_wav_path = "test/a.wav"
output_mp3_path = "test/b.mp3"
chunk_size = 12800
data, samplerate = sf.read(input_wav_path)

encoder = pylame.Encoder()
encoder.set_bitrate(128)
encoder.set_in_sample_rate(samplerate)
encoder.set_channels(1)
encoder.set_quality(7)  # 2-highest, 7-fastest

mp3_data = bytearray()
num_chunks = math.ceil(len(data) / chunk_size)
print("num_chunks:", num_chunks)
for idx, i in enumerate(range(0, len(data), chunk_size)):
    chunk = data[i : i + chunk_size]
    pcm_data = (chunk * 32767).astype(np.int16)
    mp3_chunk = encoder.encode(pcm_data)
    mp3_data.extend(mp3_chunk)
    if idx == num_chunks - 1:
        mp3_chunk = encoder.flush()
        mp3_data.extend(mp3_chunk)

with open(output_mp3_path, "wb") as mp3_file:
    mp3_file.write(mp3_data)
