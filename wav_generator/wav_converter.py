import scipy.io.wavfile as wavfile
import numpy as np
import sys
import os

def converteste_stereo_in_mono(fisier_intrare, fisier_iesire):
    if not os.path.exists(fisier_intrare):
        print(f"Eroare: Nu gasesc fisierul '{fisier_intrare}'.")
        return

    print(f"Citim '{fisier_intrare}'...")
    
    # Citim sample rate-ul (ex: 16000, 44100) si datele audio
    sample_rate, data = wavfile.read(fisier_intrare)

    # Verificam cate canale are fisierul (data.shape arata dimensiunile matricei)
    if len(data.shape) == 2 and data.shape[1] == 2:
        print("Fisier Stereo detectat. Se converteste in Mono...")
        
        # Facem media aritmetica a celor doua canale: (Stanga + Dreapta) / 2
        # Ne asiguram ca pastram exact acelasi format de date (ex: int16) cerut de Pico
        data_mono = np.mean(data, axis=1).astype(data.dtype)
        
        # Salvam noul fisier
        wavfile.write(fisier_iesire, sample_rate, data_mono)
        print(f"Succes! Noul fisier a fost salvat ca: '{fisier_iesire}' la {sample_rate} Hz.")
        
    elif len(data.shape) == 1:
        print("Fisierul este deja Mono! Nu este nevoie de nicio modificare.")
        # Doar copiem fisierul
        wavfile.write(fisier_iesire, sample_rate, data)
    else:
        print(f"Fisierul are {data.shape[1]} canale. Scriptul suporta doar Stereo (2 canale).")

if __name__ == "__main__":
    # Poti schimba numele fisierelor de aici
    nume_intrare = "bad-bunny-party-0-xvzfr.wav"
    nume_iesire = "bunny.wav"
    
    converteste_stereo_in_mono(nume_intrare, nume_iesire)