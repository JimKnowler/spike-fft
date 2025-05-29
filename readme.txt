Experiment with audio processing in raylib, and writing fft in c

DEPENDENCIES
===============================

macos

> brew install raylib

COMPILING
===============================

>> ./build.sh && ./main

CONTROLS
===============================

- 'Z' reset camera
- 'C' enable / disable mouse cursor
- 'R' restart music
- 'F' Switch between time domain and frequency domain
- 'M' Mute/Unmute

DEVELOPMENT
===============================

DONE - play mp3
DONE - capture time domain samples from audio frame
DONE    - render in 3D world as time domain signal
DONE - run fft on time domain samples in render thread
        - use different HUE for each peak
DONE - render fft as 8 octave spectrum 

TASKS
===============================

DONE - play MP3
DONE - hook up an audio processing callback
DONE - each render frame, get 512 samples from time domain
    - audio processing callback is getting 441 samples per frame, so will need to get data from 2 frames
    - read 441 from frame 1, then 71 samples from the next frame
DONE - use lock free queues to communicate between main thread + audio thread
    - audio-to-main
    - main-to-audio
DONE - initialise with 1 token on main-to-audio
    - no tokens on audio-to-main
DONE - when audio thread reads a token from main-to-audio
    - capture time domain samples to a buffer, that corresponds to the token§
    - send the token on audio-to-main queue
DONE - when main thread reads a token from audio-to-main
    - capture time domain signal
    - send token back on main-to-audio
DONE - fft
    - use 512 time domain samples to generate 512 frequency bins
    - use the first 256 frequency bins (up to nyquist)
    - generate magnitude from real/imaginary values
    - apply HANN Window
DONE - fft
    - generate 8 octaves from 256 frequencies
        - bin i = sum of pow(2, i) frequency bins



