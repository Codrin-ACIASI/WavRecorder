import wave
import struct
import math


SAMPLE_RATE = 16000
CHANNELS = 1
SAMPLE_WIDTH = 2
DURATION = 3
print("a inceput programul")
frequencies = [261.63, 296.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25]

for i in range(8):
    filename = f"REC_{i+1:03d}.WAV"
    freq =frequencies[i]

    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(CHANNELS)
        wav_file.setsampwidth(SAMPLE_WIDTH)
        wav_file.setframerate(SAMPLE_RATE)

        for num in range(DURATION * SAMPLE_RATE):
            value = int(16000 * math.sin(2 * math.pi * freq * (num / SAMPLE_RATE)))

            data = struct.pack('<h', value)
            wav_file.writeframesraw(data)




