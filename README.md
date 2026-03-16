# WavPlayer# WAV Player & Recorder – Raspberry Pi Pico 2 W

## Descriere proiect
Acest proiect constă în dezvoltarea unui sistem embedded capabil să redea și să înregistreze fișiere audio în format WAV folosind un microcontroler Raspberry Pi Pico 2 W. Sistemul citește fișiere audio de pe un card MicroSD, le procesează și le redă printr-un DAC audio conectat la un amplificator și la un difuzor sau căști.

De asemenea, sistemul poate înregistra sunet printr-un microfon și salva înregistrarea pe cardul MicroSD în format WAV.

Interacțiunea cu utilizatorul se face prin butoane fizice și printr-un display LCD grafic, care afișează informații despre fișierele audio și starea sistemului.

---

## Obiective
- Redarea fișierelor audio WAV
- Înregistrarea audio de la microfon
- Interfață grafică pentru controlul redării
- Navigarea între fișiere audio
- Controlul volumului
- Salvarea fișierelor audio pe card MicroSD

---

## Componente hardware

Lista componentelor utilizate în proiect:

- Raspberry Pi Pico 2 W
- LCD grafic ILI9341
- Modul MicroSD Card
- Card MicroSD
- Microfon analogic (ex. MAX4466 Electret Microphone Amplifier Module)
- Microfon digital I2S
- DAC audio I2S (ex. PCM5102 sau MAX98357A)
- Amplificator audio
- Speaker sau căști
- Butoane (Home, Next, Play/Pause, Prev)
- Potențiometru pentru controlul volumului
- Baterie
- Rezistențe
- Breadboard
- Fire jumper
- Cablu Micro-USB

---

## Funcționalități principale

### Redare audio
- Citirea fișierelor WAV de pe cardul MicroSD
- Decodarea header-ului WAV
- Redarea audio prin DAC și amplificator
- Redare audio fără întreruperi

### Control utilizator
- Play / Pause
- Next track
- Previous track
- Revenire la meniul principal
- Control volum prin potențiometru

### Interfață grafică
Pe display-ul LCD sunt afișate:
- numele fișierului audio
- starea redării (Play / Pause)
- durata sau progresul redării
- nivelul volumului

### Înregistrare audio
- Captarea semnalului audio de la microfon
- Conversia analog-digital (sau utilizarea microfonului I2S)
- Salvarea înregistrării în format WAV pe cardul MicroSD

---

## Cerințe funcționale

1. Sistemul trebuie să redea fișiere audio în format WAV.
2. Sistemul trebuie să citească fișierele audio de pe cardul MicroSD.
3. Utilizatorul trebuie să poată controla redarea prin butoane.
4. Sistemul trebuie să afișeze informații despre fișierul redat pe LCD.
5. Sistemul trebuie să permită controlul volumului prin potențiometru.
6. Sistemul trebuie să permită înregistrarea audio prin microfon.
7. Înregistrările trebuie salvate pe cardul MicroSD în format WAV.

---

## Cerințe non-funcționale

### Performanță
- Redare audio în timp real fără întreruperi
- Suport pentru fișiere WAV 16-bit


### Fiabilitate
- Detectarea erorilor de citire a cardului SD
- Detectarea fișierelor audio incompatibile

### Utilizabilitate
- Interfață simplă bazată pe butoane
- Informații clare afișate pe display

---

## Arhitectura sistemului

Componentele principale ale sistemului sunt:

MicroSD Card  
↓  
Raspberry Pi Pico 2 W  
↓  
DAC audio (I2S)  
↓  
Amplificator audio  
↓  
Speaker / Căști  

Interfața utilizator include:
- butoane
- potențiometru
- display LCD

---

## Interfețe utilizate

- SPI – pentru comunicarea cu LCD și modulul MicroSD
- I2S – pentru DAC audio și microfon digital
- ADC – pentru citirea potențiometrului și microfonului analogic
- GPIO – pentru butoane

---
