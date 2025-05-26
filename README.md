# SaXAudio
SaXAudio is a _**S**imple **a**udio_ library using _**XAudio**_\
It uses [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c) and [XAudio2](https://learn.microsoft.com/en-us/windows/win32/xaudio2/)

## Capabilities
- Audio bank management. Add and remove sounds to be used by voices
- Decode vorbis audio (from memory only for now)
- Create voices to play one of the sounds from the audio bank. Each voice have the following settings:
  - Volume (default 1.0)
  - Speed (default 1.0)
  - Panning (default 0.0)
  - Looping (default false)
  - LoopStart (in samples, default 0)
  - LoopEnd (in samples, default 0)
- Start a voice at a specific sample (default 0), can also be used to seek a specific position
- Stop and Pause with a fade (default 0)
- Change Volume, Speed and Panning with a fade (default 0)
- PauseAll and ResumeAll, with a fade (default 0)
- Protect a voice from PauseAll and ResumeAll
- Callback when a voice finished playing

## Usage
1. Call `BOOL success = Create();` to initialize the master voice. It'll use the default audio output device. Returns `false` if it fails.
2. Add some sounds to the audio bank with `INT32 bankID = BankAddOgg(buffer, length);`. The `buffer`must contain the vorbis encoded audio, the decoding will start immediatly asynchroneously.
3. Create a voice with `INT32 voiceID = CreateVoice(bankID);` by providing the appropriate `bankID`. If you want the sound to play immediatly use `INT32 voiceID = CreateVoice(bankID, false);` instead. Returns `-1` if it fails.
4. [Optional] Change settings of the voice before starting it. E.g. `SetVolume(voiceID, 0.5f);` or `SetLooping(voiceID, true);`
5. Start playing the voice with `BOOL success = Start(voiceID);`. Returns `false` if it fails.
6. Change a voice setting at any time during its playback. E.g. `SetSpeed(voiceID, 0.5f, 3.0f);` gradually slows down the playback to half over 3 seconds.

Note: when a voice finishes playing, it gets deleted. A unique callback can be specified using `SetOnFinishedCallback(callback);` and will be triggered anytime a voice finishes playing with the `voiceID` as argument.

## Exemple
```c++
// Create the master voice
BOOL success = Create();
if (!success)
{
    cout << "Initialization failed" << endl;
    return 1;
}

// Setup a callback, see Callback definition below
SetOnFinishedCallback(Callback);

// Load the audio file
BYTE* buffer;
UINT32 length;
// <Insert code to load the ogg file into the buffer>

// Add the audio data to the bank
INT32 bankID = BankAddOgg(buffer, length);

// Create a voice using the audio added to the bank
INT32 voiceID = CreateVoice(bankID);

// Modify some settings 
SetVolume(voiceID, 0.5f);
SetLooping(voiceID, true);

// Start playing the voice
success = Start(voiceID);
if (!success)
{
    cout << "Failed to play the sound" << endl;
    return 1;
}

```
Callback function
```c++
void Callback(INT32 voiceID)
{
    cout << "Voice " << voiceID << " finished playing" << endl;
}
```
