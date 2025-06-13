// MIT License
// 
// Copyright(c) 2025 SamsamTS
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "Includes.h"

namespace SaXAudio
{
    /// <summary>
    /// Initialize XAudio and create a mastering voice
    /// </summary>
    /// <returns>Return true if successful</returns>
    EXPORT BOOL Create();
    /// <summary>
    /// Release everything
    /// </summary>
    EXPORT void Release();

    /// <summary>
    /// Resume playing all voices
    /// </summary>
    EXPORT void StartEngine();
    /// <summary>
    /// Pause all playing voices
    /// </summary>
    EXPORT void StopEngine();

    /// <summary>
    /// Pause all playing voices
    /// </summary>
    EXPORT void PauseAll(const FLOAT fade = 0.1f);
    /// <summary>
    /// Resume playing all voices
    /// </summary>
    EXPORT void ResumeAll(const FLOAT fade = 0.1f);
    /// <summary>
    /// Protect a voice from PauseAll and ResumeAll
    /// </summary>
    /// <param name="voiceID"></param>
    EXPORT void Protect(const INT32 voiceID);

    /// <summary>
    /// Add ogg audio data to the sound bank
    /// The data in the buffer will be decoded (async) and stored in memory
    /// The buffer will not be freed/deleted
    /// </summary>
    /// <param name="buffer">The ogg data</param>
    /// <param name="length">The length in bytes of the data</param>
    /// <param name="callback">Gets called when decoding is done. Allows cleaning up resources (i.e. delete buffer)</param>
    /// <returns>unique bankID for that audio data</returns>
    EXPORT INT32 BankAddOgg(const BYTE* buffer, const UINT32 length, const OnDecodedCallback callback = nullptr);
    /// <summary>
    /// Remove and free the memory of the specified audio data
    /// </summary>
    /// <param name="bankID">The bankID of the data to remove</param>
    EXPORT void BankRemove(const INT32 bankID);

    /// <summary>
    /// Create a voice for playing the specified audio data
    /// When the audio data finished playing, the voice will be deleted
    /// </summary>
    /// <param name="bankID">The bankID of the data to play</param>
    /// <param name="paused">when false, the audio will start playing immediately</param>
    /// <returns>unique voiceID</returns>
    EXPORT INT32 CreateVoice(const INT32 bankID, const INT32 busID, const BOOL paused = true);
    /// <summary>
    /// Check if the specified voice exists
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>true if the specified voice exists</returns>
    EXPORT BOOL VoiceExist(const INT32 voiceID);

    /// <summary>
    /// Create a bus
    /// </summary>
    /// <returns>unique busID</returns>
    EXPORT INT32 CreateBus();
    /// <summary>
    /// Removes a bus
    /// </summary>
    /// <param name="busID"></param>
    EXPORT void RemoveBus(INT32 busID);

    /// <summary>
    /// Starts playing the specified voice
    /// Resets the pause stack
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>true if successful or already playing</returns>
    EXPORT BOOL Start(const INT32 voiceID);
    /// <summary>
    /// Starts playing the specified voice at a specific sample
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="sample"></param>
    /// <returns>true if successful</returns>
    EXPORT BOOL StartAtSample(const INT32 voiceID, const UINT32 sample);
    /// <summary>
    /// Starts playing the specified voice at a specific time in seconds
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    EXPORT BOOL StartAtTime(const INT32 voiceID, const FLOAT time);
    /// <summary>
    /// Stops playing the specified voice
    /// This will also delete the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    EXPORT BOOL Stop(const INT32 voiceID, const FLOAT fade = 0.1f);

    /// <summary>
    /// Pause the voice and increase the pause stack.
    /// About the pause stack: if a voice is paused more than once, it will take
    /// the same amount of resumes to for it to start playing again.
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="fade">The duration of the fade in seconds</param>
    /// <returns>value of the pause stack</returns>
    EXPORT UINT32 Pause(const INT32 voiceID, const FLOAT fade = 0.1f);
    /// <summary>
    /// Reduce the pause stack and resume playing the voice if empty
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="fade"></param>
    /// <returns>value of the pause stack</returns>
    EXPORT UINT32 Resume(const INT32 voiceID, const FLOAT fade = 0.1f);
    /// <summary>
    /// Gets the pause stack value of the specified voice, 0 if empty
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT UINT32 GetPauseStack(const INT32 voiceID);

    /// <summary>
    /// Sets the volume of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="volume">[0, 1]</param>
    EXPORT void SetVolume(const INT32 voiceID, const FLOAT volume, const FLOAT fade = 0);
    /// <summary>
    /// Sets the playback speed of the voice
    /// This will affect the pitch of the audio
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="speed"></param>
    EXPORT void SetSpeed(const INT32 voiceID, const FLOAT speed, const FLOAT fade = 0);
    /// <summary>
    /// Sets the panning of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="panning">[-1, 1]</param>
    EXPORT void SetPanning(const INT32 voiceID, const FLOAT panning, const FLOAT fade = 0);
    /// <summary>
    /// Sets if the voice should loop
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="looping"></param>
    EXPORT void SetLooping(const INT32 voiceID, const BOOL looping);
    /// <summary>
    /// Sets the start and end looping points
    /// By default the looping points are [0, last sample]
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="start">in samples</param>
    /// <param name="end">in samples</param>
    EXPORT void SetLoopPoints(const INT32 voiceID, const UINT32 start, const UINT32 end);

    /// <summary>
    /// Gets the volume of the specified voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetVolume(const INT32 voiceID);
    /// <summary>
    /// Gets the speed of the specified voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetSpeed(const INT32 voiceID);
    /// <summary>
    /// Gets the panning of the specified voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetPanning(const INT32 voiceID);
    /// <summary>
    /// Gets the specified voice is looping
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT BOOL GetLooping(const INT32 voiceID);
    /// <summary>
    /// Gets the loop start point
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT UINT32 GetLoopStart(const INT32 voiceID);
    /// <summary>
    /// Gets the loop end point
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT UINT32 GetLoopEnd(const INT32 voiceID);

    /// <summary>
    /// Add/Modify the reverb effect to a voice or bus
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="reverbParams"></param>
    /// <param name="isBus"></param>
    EXPORT void SetReverb(const INT32 voiceID, const XAUDIO2FX_REVERB_PARAMETERS reverbParams, const FLOAT fade = 0, BOOL isBus = false);
    /// <summary>
    /// Remove the reverb effect to a voice or bus
    /// </summary>
    EXPORT void RemoveReverb(const INT32 voiceID, const FLOAT fade = 0, BOOL isBus = false);

    /// <summary>
    /// Add/Modify the EQ effect to a voice or bus
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="eqParams"></param>
    /// <param name="isBus"></param>
    EXPORT void SetEq(const INT32 voiceID, const FXEQ_PARAMETERS eqParams, const FLOAT fade = 0, BOOL isBus = false);
    /// <summary>
    /// Remove the EQ effect to a voice or bus
    /// </summary>
    EXPORT void RemoveEq(const INT32 voiceID, const FLOAT fade = 0, BOOL isBus = false);

    /// <summary>
    /// Add/Modify the echo effect to a voice or bus
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="echoParams"></param>
    /// <param name="isBus"></param>
    EXPORT void SetEcho(const INT32 voiceID, const FXECHO_PARAMETERS echoParams, const FLOAT fade = 0, BOOL isBus = false);
    /// <summary>
    /// Remove the echo effect to a voice or bus
    /// </summary>
    EXPORT void RemoveEcho(const INT32 voiceID, const FLOAT fade = 0, BOOL isBus = false);

    /// <summary>
    /// Gets the position of the playing voice in samples
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>sample position if playing or 0</returns>
    EXPORT UINT32 GetPositionSample(const INT32 voiceID);
    /// <summary>
    /// Gets the position of the playing voice in seconds
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>position in seconds if playing or 0</returns>
    EXPORT FLOAT GetPositionTime(const INT32 voiceID);

    /// <summary>
    /// Gets the total amount of samples of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT UINT32 GetTotalSample(const INT32 voiceID);
    /// <summary>
    /// Gets the total time, in seconds, of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetTotalTime(const INT32 voiceID);

    /// <summary>
    /// Sets a callback for when the voice finished playing
    /// </summary>
    /// <param name="callback"></param>
    EXPORT void SetOnFinishedCallback(const OnFinishedCallback callback);
}