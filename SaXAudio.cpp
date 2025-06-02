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

#include "SaXAudio.h"

#pragma comment(lib, "xaudio2.lib")

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

namespace SaXAudio
{
    // -------------------------------------------------------
    // EXPORTS implementation
    // -------------------------------------------------------

    /// <summary>
    /// Initialize XAudio and create a mastering voice
    /// </summary>
    /// <returns>Return true if successful</returns>
    EXPORT BOOL Create()
    {
        return SaXAudio::Init();
    }

    /// <summary>
    /// Release everything
    /// </summary>
    EXPORT void Release()
    {
        SaXAudio::Release();
    }

    /// <summary>
    /// Resume playing all voices
    /// </summary>
    EXPORT void StartEngine()
    {
        SaXAudio::StartEngine();
    }

    /// <summary>
    /// Pause all playing voices
    /// </summary>
    EXPORT void StopEngine()
    {
        SaXAudio::StopEngine();
    }

    /// <summary>
    /// Pause all playing voices
    /// </summary>
    EXPORT void PauseAll(const FLOAT fade)
    {
        SaXAudio::PauseAll(fade);
    }
    /// <summary>
    /// Resume playing all voices
    /// </summary>
    EXPORT void ResumeAll(const FLOAT fade)
    {
        SaXAudio::ResumeAll(fade);
    }
    /// <summary>
    /// Protect a voice from PauseAll and ResumeAll
    /// </summary>
    /// <param name="voiceID"></param>
    EXPORT void Protect(const INT32 voiceID)
    {
        SaXAudio::Protect(voiceID);
    }

    /// <summary>
    /// Add ogg audio data to the sound bank
    /// The data will be decoded (async) and stored in memory
    /// </summary>
    /// <param name="buffer">The ogg data</param>
    /// <param name="length">The length in bytes of the data</param>
    /// <returns>unique bankID for that audio data</returns>
    EXPORT INT32 BankAddOgg(const BYTE* buffer, const UINT32 length, const OnDecodedCallback callback)
    {
        AudioData* data = new AudioData;
        data->onDecodedCallback = callback;
        INT32 bankID = SaXAudio::Add(data);
        if (bankID >= 0)
            SaXAudio::StartDecodeOgg(bankID, buffer, length);
        return bankID;
    }
    /// <summary>
    /// Remove and free the memory of the specified audio data
    /// </summary>
    /// <param name="bankID">The bankID of the data to remove</param>
    EXPORT void BankRemove(const INT32 bankID)
    {
        SaXAudio::Remove(bankID);
    }

    /// <summary>
    /// Create a voice for playing the specified audio data
    /// When the audio data finished playing, the voice will be deleted
    /// </summary>
    /// <param name="bankID">The bankID of the data to play</param>
    /// <param name="paused">when false, the audio will start playing immediately</param>
    /// <returns>unique voiceID</returns>
    EXPORT INT32 CreateVoice(const INT32 bankID, const INT32 busID, const BOOL paused)
    {
        AudioVoice* voice = SaXAudio::CreateVoice(bankID, busID);

        if (!voice)
            return -1;

        if (!paused)
            voice->Start();

        return voice->VoiceID;
    }

    /// <summary>
    /// Check if the specified voice exists
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>true if the specified voice exists</returns>
    EXPORT BOOL VoiceExist(const INT32 voiceID)
    {
        return SaXAudio::GetVoice(voiceID) != nullptr;
    }

    /// <summary>
    /// Create a bus
    /// </summary>
    /// <returns>unique busID</returns>
    EXPORT INT32 CreateBus()
    {
        return SaXAudio::AddBus();
    }

    /// <summary>
    /// Removes a bus
    /// </summary>
    /// <param name="busID"></param>
    EXPORT void RemoveBus(INT32 busID)
    {
        SaXAudio::RemoveBus(busID);
    }

    /// <summary>
    /// Starts playing the specified voice
    /// Resets the pause stack
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>true if successful or already playing</returns>
    EXPORT BOOL Start(const INT32 voiceID)
    {
        return StartAtSample(voiceID, 0);
    }

    /// <summary>
    /// Starts playing the specified voice at a specific sample
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="sample"></param>
    /// <returns>true if successful</returns>
    EXPORT BOOL StartAtSample(const INT32 voiceID, const UINT32 sample)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Start(sample);
        }
        return false;
    }

    /// <summary>
    /// Starts playing the specified voice at a specific time in seconds
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    EXPORT BOOL StartAtTime(const INT32 voiceID, const FLOAT time)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice && voice->BankData && time >= 0)
        {
            UINT32 sample = (UINT32)(time * voice->BankData->sampleRate);
            return voice->Start(sample);
        }
        return false;
    }

    /// <summary>
    /// Stops playing the specified voice
    /// This will also delete the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    EXPORT BOOL Stop(const INT32 voiceID, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Stop(fade);
        }
        return false;
    }

    /// <summary>
    /// Pause the voice and increase the pause stack.
    /// About the pause stack: if a voice is paused more than once, it will take
    /// the same amount of resumes to for it to start playing again.
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="fade">The duration of the fade in seconds</param>
    /// <returns>value of the pause stack</returns>
    EXPORT UINT32 Pause(const INT32 voiceID, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Pause(fade);
        }
        return 0;
    }

    /// <summary>
    /// Reduce the pause stack and resume playing the voice if empty
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="fade"></param>
    /// <returns>value of the pause stack</returns>
    EXPORT UINT32 Resume(const INT32 voiceID, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Resume(fade);
        }
        return 0;
    }

    /// <summary>
    /// Gets the pause stack value of the specified voice, 0 if empty
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT UINT32 GetPauseStack(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->GetPauseStack();
        }
        return 0;
    }

    /// <summary>
    /// Sets the volume of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="volume">[0, 1]</param>
    EXPORT void SetVolume(const INT32 voiceID, const FLOAT volume, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            voice->SetVolume(volume, fade);
        }
    }

    /// <summary>
    /// Sets the playback speed of the voice
    /// This will affect the pitch of the audio
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="speed"></param>
    EXPORT void SetSpeed(const INT32 voiceID, const FLOAT speed, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            voice->SetSpeed(speed, fade);
        }
    }

    /// <summary>
    /// Sets the panning of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="panning">[-1, 1]</param>
    EXPORT void SetPanning(const INT32 voiceID, const FLOAT panning, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            voice->SetPanning(panning, fade);
        }
    }

    /// <summary>
    /// Sets if the voice should loop
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="looping"></param>
    EXPORT void SetLooping(const INT32 voiceID, const BOOL looping)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            if (looping == voice->Looping)
                return;

            if (!voice->IsPlaying)
            {
                voice->Looping = looping;
                return;
            }

            if (looping)
            {
                // Start looping
                voice->Looping = true;
                voice->ChangeLoopPoints(voice->LoopStart, voice->LoopEnd);
            }
            else if (voice->SourceVoice)
            {
                // This will stop the loop while continuing the playback from current position
                voice->ChangeLoopPoints(0, 0);
                voice->Looping = false;
            }
        }
    }

    /// <summary>
    /// Sets the start and end looping points
    /// By default the looping points are [0, last sample]
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="start">in samples</param>
    /// <param name="end">in samples</param>
    EXPORT void SetLoopPoints(const INT32 voiceID, const UINT32 start, const UINT32 end)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            voice->ChangeLoopPoints(start, end);
        }
    }

    /// <summary>
    /// Gets the volume of the specified voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetVolume(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Volume;
        }
        return 1.0f;
    }

    /// <summary>
    /// Gets the speed of the specified voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetSpeed(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Speed;
        }
        return 1.0f;
    }

    /// <summary>
    /// Gets the panning of the specified voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetPanning(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Panning;
        }
        return 0.0f;
    }

    /// <summary>
    /// Gets the specified voice is looping
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT BOOL GetLooping(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Looping;
        }
        return false;
    }

    EXPORT void SetReverb(const INT32 voiceID, const XAUDIO2FX_REVERB_PARAMETERS reverbParams)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (!voice || !voice->SourceVoice) return;

        voice->EffectData.reverb = reverbParams;
        SaXAudio::SetReverb(voice->SourceVoice, &voice->EffectData);
    }

    EXPORT void SetReverbBus(const INT32 busID, const XAUDIO2FX_REVERB_PARAMETERS reverbParams)
    {
        BusData* bus = SaXAudio::GetBus(busID);
        if (!bus || !bus->voice) return;

        bus->reverb = reverbParams;
        SaXAudio::SetReverb(bus->voice, bus);
    }

    EXPORT void RemoveReverb(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (!voice || !voice->SourceVoice) return;
        SaXAudio::RemoveReverb(voice->SourceVoice, &voice->EffectData);
    }

    EXPORT void RemoveReverbBus(const INT32 busID)
    {
        BusData* bus = SaXAudio::GetBus(busID);
        if (!bus || !bus->voice) return;
        SaXAudio::RemoveReverb(bus->voice, bus);
    }

    EXPORT void SetEq(const INT32 voiceID, const FXEQ_PARAMETERS eqParams)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (!voice || !voice->SourceVoice) return;

        voice->EffectData.eq = eqParams;
        SaXAudio::SetEq(voice->SourceVoice, &voice->EffectData);
    }

    EXPORT void SetEqBus(const INT32 busID, const FXEQ_PARAMETERS eqParams)
    {
        BusData* bus = SaXAudio::GetBus(busID);
        if (!bus || !bus->voice) return;

        bus->eq = eqParams;
        SaXAudio::SetEq(bus->voice, bus);
    }

    EXPORT void RemoveEq(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (!voice || !voice->SourceVoice) return;
        SaXAudio::RemoveEq(voice->SourceVoice, &voice->EffectData);
    }

    EXPORT void RemoveEqBus(const INT32 busID)
    {
        BusData* bus = SaXAudio::GetBus(busID);
        if (!bus || !bus->voice) return;
        SaXAudio::RemoveEq(bus->voice, bus);
    }

    /// <summary>
    /// Gets the position of the playing voice in samples
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>sample position if playing or 0</returns>
    EXPORT UINT32 GetPositionSample(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->GetPosition();
        }
        return 0;
    }

    /// <summary>
    /// Gets the position of the playing voice in seconds
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>position in seconds if playing or 0</returns>
    EXPORT FLOAT GetPositionTime(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return (FLOAT)voice->GetPosition() / voice->BankData->sampleRate;
        }
        return 0.0f;
    }

    /// <summary>
    /// Gets the total amount of samples of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT UINT32 GetTotalSample(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return voice->BankData->totalSamples;
        }
        return 0;
    }

    /// <summary>
    /// Gets the total time, in seconds, of the voice
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns></returns>
    EXPORT FLOAT GetTotalTime(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return (FLOAT)voice->BankData->totalSamples / voice->BankData->sampleRate;
        }
        return 0.0f;
    }

    /// <summary>
    /// Sets a callback for when the voice finished playing
    /// </summary>
    /// <param name="callback"></param>
    EXPORT void SetOnFinishedCallback(const OnFinishedCallback callback)
    {
        Log(-1, -1, "[OnVoiceFinished]");
        SaXAudio::OnFinishedCallback = callback;
    }

    // -------------------------------------------------------
    // AudioVoice class implementation
    // -------------------------------------------------------

    BOOL AudioVoice::Start(const UINT32 atSample, BOOL flush)
    {
        if (!SourceVoice || !BankData) return false;
        Log(BankID, VoiceID, "[Start] at: " + to_string(atSample) + (Looping ? " loop start: " + to_string(LoopStart) + " loop end: " + to_string(LoopEnd) : ""));

        if (IsPlaying)
        {
            if (flush)
            {
                m_tempFlush++;
                SourceVoice->Stop();
                SourceVoice->FlushSourceBuffers();
            }

            // Update position offset
            XAUDIO2_VOICE_STATE state;
            SourceVoice->GetState(&state);
            m_positionOffset = state.SamplesPlayed - atSample;
        }

        // Set up buffer
        Buffer.PlayBegin = atSample;
        Buffer.PlayLength = 0; // Plays until the end

        // Check if we are past the new looping points
        if (Looping && atSample < LoopEnd)
        {
            Buffer.LoopBegin = LoopStart;
            Buffer.LoopLength = LoopEnd - LoopStart;
            Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        }
        else
        {
            // XAudio will refuse the buffer if PlayBegin is past the end of the loop,
            // we will just play the sound from the position without a loop
            Buffer.LoopBegin = 0;
            Buffer.LoopLength = 0;
            Buffer.LoopCount = 0;
        }

        // Submitting the buffer
        if (FAILED(SourceVoice->SubmitSourceBuffer(&Buffer)))
        {
            Log(BankID, VoiceID, "[Start] Failed to submit buffer");
            SaXAudio::RemoveVoice(VoiceID);
            return false;
        }

        IsPlaying = true;

        if (m_pauseStack > 0)
        {
            Log(BankID, VoiceID, "[Start] Voice paused");
            return IsPlaying;
        }

        if (BankData->decodedSamples <= atSample)
        {
            // Waiting for some decoded samples to not read garbage
            thread wait(WaitForDecoding, this);
            wait.detach();
        }
        else if (FAILED(SourceVoice->Start()))
        {
            Log(BankID, VoiceID, "[Start] FAILED starting");
            SaXAudio::RemoveVoice(VoiceID);
            return false;
        }
        return IsPlaying;
    }

    void AudioVoice::WaitForDecoding(AudioVoice* voice)
    {
        Log(voice->BankID, voice->VoiceID, "[Start] Waiting for decoded data");
        unique_lock<mutex> lock(voice->BankData->decodingMutex);
        if (voice->BankData->decodedSamples == 0)
        {
            if (!voice->BankData->decodingPerform.wait_for(lock, chrono::milliseconds(500), [voice] { return voice->BankData->decodedSamples > voice->Buffer.PlayBegin; }))
            {
                Log(voice->BankID, voice->VoiceID, "[Start] FAILED Waiting for decoded data timed out");
                SaXAudio::RemoveVoice(voice->VoiceID);
                return;

            }
        }
        if (FAILED(voice->SourceVoice->Start()))
        {
            Log(voice->BankID, voice->VoiceID, "[Start] FAILED starting");
            SaXAudio::RemoveVoice(voice->VoiceID);
        }
        else
        {
            voice->m_tempFlush = false;
            Log(voice->BankID, voice->VoiceID, "[Start] Successfully waited for decoded data");
        }
    }

    BOOL AudioVoice::Stop(const FLOAT fade)
    {
        if (!SourceVoice || !IsPlaying) return false;
        Log(BankID, VoiceID, "[Stop] fade: " + to_string(fade));

        IsPlaying = false;
        Looping = false;

        if (fade > 0)
        {
            FadeVolume(0, fade);
            m_fadeInfo.pause = false;
            m_fadeInfo.stop = true;
        }
        else
        {
            m_fadeInfo.pause = false;
            m_fadeInfo.stop = false;
            m_fading = false;
            m_tempFlush = 0;
            SourceVoice->Stop();
            SourceVoice->FlushSourceBuffers();
        }

        return true;
    }

    UINT32 AudioVoice::Pause(const FLOAT fade)
    {
        if (!SourceVoice) return 0;

        m_pauseStack++;
        Log(BankID, VoiceID, "[Pause] stack: " + to_string(m_pauseStack));

        if (!IsPlaying && !m_fadeInfo.stop)
            return m_pauseStack;

        if (fade > 0 && Volume > 0 && !m_fadeInfo.stop)
        {
            FadeVolume(0, fade);
            m_fadeInfo.pause = true;
        }
        else
        {
            m_fadeInfo.pause = false;
            if (m_fading && m_fadeInfo.stop)
                m_fading = false;
            else
                m_fadeInfo.volumeRate = 0;
            SourceVoice->Stop();
        }
        return m_pauseStack;
    }

    UINT32 AudioVoice::Resume(const FLOAT fade)
    {
        if (!SourceVoice || m_pauseStack == 0) return 0;

        m_pauseStack--;
        Log(BankID, VoiceID, "[Resume] stack: " + to_string(m_pauseStack) + (Looping ? " - Looping" : ""));

        if (m_pauseStack > 0)
            return m_pauseStack;

        SourceVoice->Start();
        m_fadeInfo.pause = false;

        if (fade > 0 && Volume > 0)
        {
            FLOAT target = Volume;
            Volume = 0;
            FadeVolume(target, fade);
            Volume = target;
        }
        else if (m_fadeInfo.volumeRate != 0 || m_fadeInfo.speedRate != 0 || m_fadeInfo.panningRate != 0)
        {
            // Resume the fading interrupted by pause
            m_fading = true;
            thread fade(Fade, this);
            fade.detach();
        }

        if (fade == 0)
        {
            m_fadeInfo.volumeRate = 0;
            SourceVoice->SetVolume(Volume);
        }
        return m_pauseStack;
    }

    UINT32 AudioVoice::GetPauseStack()
    {
        return m_pauseStack;
    }

    UINT32 AudioVoice::GetPosition()
    {
        if (!IsPlaying || !SourceVoice) return 0;

        XAUDIO2_VOICE_STATE state;
        SourceVoice->GetState(&state);

        // We might not have started at from the beginning of the buffer so we need add PlayBegin
        UINT64 position = state.SamplesPlayed - m_positionOffset + Buffer.PlayBegin;
        if (Looping && position > LoopStart)
        {
            // We are somewhere in the loop
            position = LoopStart + ((position - LoopStart) % (LoopEnd - LoopStart));
        }

        if (position == 0)
            return 1; // Probably waiting on decoding, returning 0 would mean it finished playing

        return (INT32)position;
    }

    void AudioVoice::ChangeLoopPoints(const UINT32 start, UINT32 end)
    {
        if (!SourceVoice || !BankData) return;
        Log(BankID, VoiceID, "[ChangeLoopPoints] start: " + to_string(start) + " end: " + to_string(end));

        // When start and end equals 0 the loop will be stopped
        if (end == 0 && start != 0)
            end = BankData->totalSamples - 1;

        if (LoopStart == start && LoopEnd == end)
            return;

        UINT64 position = 0;
        if (IsPlaying)
        {
            // We need to stop the voice to change the looping points
            SourceVoice->Stop();

            // We will calculate the current position in the buffer
            // Unfortunately SamplesPlayed can be slightly inaccurate if the speed has been modified
            // With loops this may add up
            XAUDIO2_VOICE_STATE state;
            SourceVoice->GetState(&state);

            // Temporary flush the buffer
            m_tempFlush++;
            SourceVoice->FlushSourceBuffers();

            // We might not have started at from the beginning of the buffer so we need add PlayBegin
            position = state.SamplesPlayed - m_positionOffset + Buffer.PlayBegin;
            if (Looping)
            {
                if (position > LoopStart)
                {
                    // We are somewhere in the loop
                    position = LoopStart + ((position - LoopStart) % (LoopEnd - LoopStart));
                }
            }
        }

        // Make sure LoopStart < LoopEnd
        if (start < end)
        {
            LoopStart = start;
            LoopEnd = end;
        }
        else
        {
            LoopStart = end;
            LoopEnd = start;
        }

        if (LoopStart == LoopEnd && LoopEnd != 0)
            LoopStart = LoopEnd - 1;

        if (!IsPlaying)
            return;

        // Resume playing
        Start((UINT32)position, false);
    }

    void AudioVoice::SetVolume(const FLOAT volume, const FLOAT fade)
    {
        if (!SourceVoice || Volume == volume) return;
        Log(BankID, VoiceID, "[SetVolume] to: " + to_string(volume) + " fade: " + to_string(fade));

        if (fade > 0)
        {
            FadeVolume(volume, fade);
        }
        else
        {
            m_fadeInfo.volumeRate = 0;
            SourceVoice->SetVolume(volume);
        }
        Volume = volume;
    }

    void AudioVoice::SetSpeed(FLOAT speed, const FLOAT fade)
    {
        if (!SourceVoice || !BankData || Speed == speed) return;
        Log(BankID, VoiceID, "[SetSpeed] to: " + to_string(speed) + " fade: " + to_string(fade));

        // Ensure ratio is not too small
        if (speed < XAUDIO2_MIN_FREQ_RATIO)
            speed = XAUDIO2_MIN_FREQ_RATIO;

        if (fade > 0)
        {
            FadeSpeed(speed, fade);
        }
        else
        {
            m_fadeInfo.speedRate = 0;
            HRESULT hr = SourceVoice->SetFrequencyRatio(speed);
            if (FAILED(hr))
            {
                Log(BankID, VoiceID, "[SetSpeed] FAILED setting speed to " + to_string(speed));
            }
        }
        Speed = speed;
    }

    void AudioVoice::SetPanning(const FLOAT panning, const FLOAT fade)
    {
        if (!SourceVoice || Panning == panning) return;
        Log(BankID, VoiceID, "[SetPanning] to: " + to_string(panning) + " fade: " + to_string(fade));

        if (fade > 0)
        {
            FadePanning(panning, fade);
        }
        else
        {
            m_fadeInfo.panningRate = 0;
            SetOutputMatrix(panning);
        }
        Panning = panning;
    }

    // Fast approximation of cosine using Taylor series (good for [0, π/2])
    inline FLOAT FastCos(const FLOAT x)
    {
        // Normalize to [0, π/2] and use Taylor series: cos(x) ≈ 1 - x²/2 + x⁴/24 - x⁶/720
        FLOAT x2 = x * x;
        FLOAT x4 = x2 * x2;
        FLOAT x6 = x4 * x2;
        return 1.0f - x2 * 0.5f + x4 * 0.041666667f - x6 * 0.001388889f;
    }

    // Fast approximation of sine using Taylor series (good for [0, π/2])
    inline FLOAT FastSin(const FLOAT x)
    {
        // Taylor series: sin(x) ≈ x - x³/6 + x⁵/120 - x⁷/5040
        FLOAT x2 = x * x;
        FLOAT x3 = x2 * x;
        FLOAT x5 = x3 * x2;
        FLOAT x7 = x5 * x2;
        return x - x3 * 0.166666667f + x5 * 0.008333333f - x7 * 0.000198413f;
    }

    void AudioVoice::SetOutputMatrix(FLOAT panning)
    {
        if (!SourceVoice || !BankData) return;

        UINT32 sourceChannels = BankData->channels;
        UINT32 destChannels = SaXAudio::m_masterDetails.InputChannels;
        DWORD channelMask = SaXAudio::m_channelMask;
        if (sourceChannels > 2 || destChannels == 0)
            return;

        // Clamp panning to valid range
        panning = (panning < -1.0f) ? -1.0f : ((panning > 1.0f) ? 1.0f : panning);

        // Treat mono as stereo (2 identical channels)
        const UINT32 inChannels = 2;
        // Max out channels
        const UINT32 outChannels = 12;
        if (destChannels > outChannels)
            destChannels = outChannels;

        // Allocate output matrix on stack (assume reasonable max channels)
        FLOAT outputMatrix[inChannels * outChannels] = { 0 }; // Max 2x12 matrix, initialized to 0
        UINT32 matrixSize = inChannels * destChannels;
        if (matrixSize > inChannels * outChannels) return; // Safety check

        // Calculate pan gains using fast constant power panning
        FLOAT leftGain, rightGain;
        FLOAT panAngle = (panning + 1.0f) * 0.25f * 3.14159265359f; // Map [-1,1] to [0, π/2]
        leftGain = FastCos(panAngle);
        rightGain = FastSin(panAngle);

        // Find channel positions in destination (support 5.1/7.1)
        int leftChannelIndex = -1;
        int rightChannelIndex = -1;
        int centerChannelIndex = -1;
        int lfeChannelIndex = -1;
        int backLeftChannelIndex = -1;
        int backRightChannelIndex = -1;
        int sideLeftChannelIndex = -1;
        int sideRightChannelIndex = -1;

        // Map channel mask to indices
        DWORD currentMask = 1;
        int channelIndex = 0;

        for (UINT32 i = 0; i < outChannels && channelIndex < (int)destChannels; i++)
        {
            if (channelMask & currentMask)
            {
                switch (currentMask)
                {
                case SPEAKER_FRONT_LEFT: leftChannelIndex = channelIndex; break;
                case SPEAKER_FRONT_RIGHT: rightChannelIndex = channelIndex; break;
                case SPEAKER_FRONT_CENTER: centerChannelIndex = channelIndex; break;
                case SPEAKER_LOW_FREQUENCY: lfeChannelIndex = channelIndex; break;
                case SPEAKER_BACK_LEFT: backLeftChannelIndex = channelIndex; break;
                case SPEAKER_BACK_RIGHT: backRightChannelIndex = channelIndex; break;
                case SPEAKER_SIDE_LEFT: sideLeftChannelIndex = channelIndex; break;
                case SPEAKER_SIDE_RIGHT: sideRightChannelIndex = channelIndex; break;
                }
                channelIndex++;
            }
            currentMask <<= 1;
        }

        // If we don't have explicit left/right channels, use first two channels
        if (leftChannelIndex == -1 && destChannels >= 1) leftChannelIndex = 0;
        if (rightChannelIndex == -1 && destChannels >= 2) rightChannelIndex = 1;
        else if (rightChannelIndex == -1) rightChannelIndex = leftChannelIndex; // Mono output

        // If we don't have explicit left/right channels, use first two channels
        if (leftChannelIndex == -1 && destChannels >= 1) leftChannelIndex = 0;
        if (rightChannelIndex == -1 && destChannels >= 2) rightChannelIndex = 1;
        else if (rightChannelIndex == -1) rightChannelIndex = leftChannelIndex; // Mono output

        // Set up the matrix based on source configuration
        if (sourceChannels == 1)
        {
            // Mono source: single channel matrix
            // Apply panning to front L/R channels
            if (leftChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + leftChannelIndex] = leftGain;
            }
            if (rightChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + rightChannelIndex] = rightGain;
            }

            // Send to center channel (if present)
            if (centerChannelIndex >= 0)
            {
                FLOAT centerGain = 0.707f; // No summing, so use -3dB
                outputMatrix[0 * destChannels + centerChannelIndex] = centerGain;
            }

            // Send to surround channels at reduced level
            FLOAT surroundGain = 0.5f; // -6dB
            if (backLeftChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + backLeftChannelIndex] = surroundGain;
            }
            if (backRightChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + backRightChannelIndex] = surroundGain;
            }
            if (sideLeftChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + sideLeftChannelIndex] = surroundGain;
            }
            if (sideRightChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + sideRightChannelIndex] = surroundGain;
            }
        }
        else if (sourceChannels == 2)
        {
            // Stereo source: two channel matrix
            // Apply panning to front L/R channels
            if (leftChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + leftChannelIndex] = leftGain;   // Left -> Left
                outputMatrix[1 * destChannels + leftChannelIndex] = leftGain;   // Right -> Left
            }
            if (rightChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + rightChannelIndex] = rightGain; // Left -> Right
                outputMatrix[1 * destChannels + rightChannelIndex] = rightGain; // Right -> Right
            }

            // Mix to center channel (if present) - sum L+R at reduced level
            if (centerChannelIndex >= 0)
            {
                FLOAT centerGain = 0.5f; // -6dB to prevent overload when summing L+R
                outputMatrix[0 * destChannels + centerChannelIndex] = centerGain; // Left -> Center
                outputMatrix[1 * destChannels + centerChannelIndex] = centerGain; // Right -> Center
            }

            // Send to surround channels at reduced level (ambient effect)
            FLOAT surroundGain = 0.35f; // -9dB
            if (backLeftChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + backLeftChannelIndex] = surroundGain; // Left -> Back Left
            }
            if (backRightChannelIndex >= 0)
            {
                outputMatrix[1 * destChannels + backRightChannelIndex] = surroundGain; // Right -> Back Right
            }
            if (sideLeftChannelIndex >= 0)
            {
                outputMatrix[0 * destChannels + sideLeftChannelIndex] = surroundGain; // Left -> Side Left
            }
            if (sideRightChannelIndex >= 0)
            {
                outputMatrix[1 * destChannels + sideRightChannelIndex] = surroundGain; // Right -> Side Right
            }
        }

        // LFE channel gets no direct signal (would need bass management for proper implementation)

        // Apply the output matrix to the voice
        HRESULT hr = SourceVoice->SetOutputMatrix(nullptr, sourceChannels, destChannels, outputMatrix);
        if (FAILED(hr))
        {
            Log(BankID, VoiceID, "[SetOutputMatrix] FAILED");
        }
    }

    void AudioVoice::Reset()
    {
        m_pauseStack = 0;
        m_positionOffset = 0;
        m_volumeTarget = 0;
        m_fadeInfo = {};
        m_fading = false;

        Buffer = { 0 };
        BankID = -1;

        SourceVoice = nullptr;
        BankData = nullptr;
        Volume = 1.0f;
        Speed = 1.0f;
        Panning = 0.0f;
        LoopStart = 0;
        LoopEnd = 0;
        Looping = false;
        IsPlaying = false;
    }

    inline FLOAT AudioVoice::GetRate(const FLOAT from, const FLOAT to, const FLOAT duration)
    {
        return (to - from) / (duration * 100.0f);
    }

    inline FLOAT MoveToTarget(const FLOAT start, const FLOAT end, const FLOAT rate)
    {
        if (rate > 0 && start < end)
        {
            FLOAT result = start + rate;
            return result > end ? end : result;
        }
        if (rate < 0 && start > end)
        {
            FLOAT result = start + rate;
            return result < end ? end : result;
        }
        // Rate would move away from target
        Log(-1, -1, "[MoveToTarget] Invalid rate");
        return end;
    }

    void AudioVoice::Fade(AudioVoice* voice)
    {
        if (!voice->SourceVoice || !voice->BankData)
        {
            voice->m_fading = false;
            return;
        }

        Log(voice->BankID, voice->VoiceID, "[Fade] Fading" +
            (voice->m_fadeInfo.volumeRate != 0 ? " volumeRate: " + to_string(voice->m_fadeInfo.volumeRate) : "") +
            (voice->m_fadeInfo.speedRate != 0 ? " speedRate: " + to_string(voice->m_fadeInfo.speedRate) : "") +
            (voice->m_fadeInfo.panningRate != 0 ? " panningRate: " + to_string(voice->m_fadeInfo.panningRate) : ""));

        auto start = chrono::steady_clock::now();
        chrono::milliseconds interval = chrono::milliseconds(10);
        UINT64 count = 0;

        do
        {
            count++;
            auto target_time = start + (interval * count);
            this_thread::sleep_until(target_time);

            // Volume
            if (voice->m_fadeInfo.volumeRate != 0)
            {
                voice->m_fadeInfo.volume = MoveToTarget(voice->m_fadeInfo.volume, voice->m_fadeInfo.volumeTarget, voice->m_fadeInfo.volumeRate);
                if (voice->m_fadeInfo.volume == voice->m_fadeInfo.volumeTarget)
                    voice->m_fadeInfo.volumeRate = 0;
                voice->SourceVoice->SetVolume(voice->m_fadeInfo.volume);
            }

            // Speed
            if (voice->m_fadeInfo.speedRate != 0)
            {
                voice->m_fadeInfo.speed = MoveToTarget(voice->m_fadeInfo.speed, voice->m_fadeInfo.speedTarget, voice->m_fadeInfo.speedRate);
                if (voice->m_fadeInfo.speed == voice->m_fadeInfo.speedTarget)
                    voice->m_fadeInfo.speedRate = 0;
                voice->SourceVoice->SetFrequencyRatio(voice->m_fadeInfo.speed);
            }

            // Panning
            if (voice->m_fadeInfo.panningRate != 0)
            {
                voice->m_fadeInfo.panning = MoveToTarget(voice->m_fadeInfo.panning, voice->m_fadeInfo.panningTarget, voice->m_fadeInfo.panningRate);
                if (voice->m_fadeInfo.panning == voice->m_fadeInfo.panningTarget)
                    voice->m_fadeInfo.panningRate = 0;
                voice->SetOutputMatrix(voice->m_fadeInfo.panning);
            }
        }
        while (voice->m_fading && voice->SourceVoice && voice->BankData && (
            voice->m_fadeInfo.volumeRate != 0 ||
            (voice->m_fadeInfo.speedRate != 0 && !voice->m_fadeInfo.pause && !voice->m_fadeInfo.stop) ||
            (voice->m_fadeInfo.panningRate != 0 && !voice->m_fadeInfo.pause && !voice->m_fadeInfo.stop)));

        if (!voice->m_fading)
            return;

        if (voice->m_fadeInfo.volumeRate == 0 && voice->m_fadeInfo.speedRate == 0 && voice->m_fadeInfo.panningRate == 0)
        {
            Log(voice->BankID, voice->VoiceID, "[Fade] Fading complete");
        }

        if (voice->m_fadeInfo.pause)
        {
            Log(voice->BankID, voice->VoiceID, "[Fade] Pausing");
            voice->SourceVoice->Stop();
            voice->m_fadeInfo.pause = false;
        }

        if (voice->m_fadeInfo.stop && voice->m_fadeInfo.volumeRate == 0)
        {
            Log(voice->BankID, voice->VoiceID, "[Fade] Stopping");
            voice->IsPlaying = true;
            voice->Stop();
        }
        voice->m_fading = false;
    }

    void AudioVoice::FadeVolume(const FLOAT target, const FLOAT duration)
    {
        m_fadeInfo.volumeTarget = target;
        if (m_fading)
        {
            m_fadeInfo.volumeRate = GetRate(m_fadeInfo.volume, m_fadeInfo.volumeTarget, duration);
        }
        else
        {
            m_fading = true;
            m_fadeInfo.volumeRate = GetRate(Volume, m_fadeInfo.volumeTarget, duration);
            m_fadeInfo.volume = Volume;
            thread fade(Fade, this);
            fade.detach();
        }
    }

    void AudioVoice::FadeSpeed(const FLOAT target, const FLOAT duration)
    {
        m_fadeInfo.speedTarget = target;
        if (m_fading)
        {
            m_fadeInfo.speedRate = GetRate(m_fadeInfo.speed, m_fadeInfo.speedTarget, duration);
        }
        else
        {
            m_fading = true;
            m_fadeInfo.speedRate = GetRate(Speed, m_fadeInfo.speedTarget, duration);
            m_fadeInfo.speed = Speed;
            thread fade(Fade, this);
            fade.detach();
        }
    }

    void AudioVoice::FadePanning(const FLOAT target, const FLOAT duration)
    {
        m_fadeInfo.panningTarget = target;
        if (m_fading)
        {
            m_fadeInfo.panningRate = GetRate(m_fadeInfo.panning, m_fadeInfo.panningTarget, duration);
        }
        else
        {
            m_fading = true;
            m_fadeInfo.panningRate = GetRate(Panning, m_fadeInfo.panningTarget, duration);
            m_fadeInfo.panning = Panning;
            thread fade(Fade, this);
            fade.detach();
        }
    }

    void __stdcall AudioVoice::OnBufferEnd(void* pBufferContext)
    {
        // We don't want to do anything when temporary flushing the buffer
        if (m_tempFlush > 0)
        {
            Log(BankID, VoiceID, "[OnBufferEnd] Flush reset");
            m_tempFlush--;
            return;
        }
        Log(BankID, VoiceID, "[OnBufferEnd] Voice finished playing");

        IsPlaying = false;
        SaXAudio::RemoveVoice(VoiceID);

        if (SaXAudio::OnFinishedCallback != nullptr)
        {
            thread onFinished(*SaXAudio::OnFinishedCallback, VoiceID);
            onFinished.detach();
        }
    }

    // -------------------------------------------------------
    // SaXAudio class implementation
    // -------------------------------------------------------

    IXAudio2* SaXAudio::m_XAudio = nullptr;
    IXAudio2MasteringVoice* SaXAudio::m_masteringVoice = nullptr;

    unordered_map<INT32, AudioData*> SaXAudio::m_bank;
    INT32 SaXAudio::m_bankCounter = 0;
    mutex SaXAudio::m_bankMutex;

    unordered_map<INT32, AudioVoice*> SaXAudio::m_voices;
    INT32 SaXAudio::m_voiceCounter = 0;
    mutex SaXAudio::m_voiceMutex;

    unordered_map<INT32, BusData> SaXAudio::m_buses;
    INT32 SaXAudio::m_busCounter = 0;
    mutex SaXAudio::m_busMutex;

    DWORD SaXAudio::m_channelMask = 0;
    XAUDIO2_VOICE_DETAILS SaXAudio::m_masterDetails = { 0 };

    OnFinishedCallback SaXAudio::OnFinishedCallback = nullptr;

    BOOL SaXAudio::Init()
    {
        StartLogging();

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        switch (hr)
        {
        case S_OK:
        case S_FALSE:
            break;
        case RPC_E_CHANGED_MODE:
            // COM initialized with different threading model
            // Try apartment threading instead
            hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            if (SUCCEEDED(hr))
                break;
        default:
            Log(-1, -1, "[Init] COM initialize failed");
            return false;
        }

        // Create XAudio2 instance
        hr = XAudio2Create(&SaXAudio::m_XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr))
        {
            Log(-1, -1, "[Init] XAudio2 creation failed");
            return false;
        }


        // Create mastering voice
        hr = m_XAudio->CreateMasteringVoice(&m_masteringVoice);
        if (FAILED(hr))
        {
            Log(-1, -1, "[Init] Mastering voice creation failed");
            m_XAudio->Release();
            return false;
        }

        // Get channel mask
        hr = m_masteringVoice->GetChannelMask(&m_channelMask);
        if (FAILED(hr))
        {
            Log(-1, -1, "[Init] Couldn't get channel mask");
            m_XAudio->Release();
            return false;
        }

        // Get details
        m_masteringVoice->GetVoiceDetails(&m_masterDetails);
        Log(-1, -1, "[Init] Initialization complete. Channels: " + to_string(m_masterDetails.InputChannels) + " Sample rate: " + to_string(m_masterDetails.InputSampleRate));

        return true;
    }

    void SaXAudio::Release()
    {
        lock_guard<mutex> lock(m_bankMutex);

        if (m_XAudio)
        {
            m_XAudio->Release();
        }

        for (auto it = m_bank.begin(); it != m_bank.end();)
        {
            INT32 bankID = it->first;
            it++;
            Remove(bankID);
        }
        StopLogging();
    }

    void SaXAudio::StopEngine()
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        Log(-1, -1, "[StopEngine]");

        m_XAudio->StopEngine();
    }

    void SaXAudio::StartEngine()
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        Log(-1, -1, "[StartEngine]");

        m_XAudio->StartEngine();
    }

    void SaXAudio::PauseAll(const FLOAT fade)
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        Log(-1, -1, "[PauseAll]");

        for (auto& it : m_voices)
        {
            if (!it.second->IsProtected)
                it.second->Pause(fade);
        }
    }

    void SaXAudio::ResumeAll(const FLOAT fade)
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        Log(-1, -1, "[ResumeAll]");

        for (auto& it : m_voices)
        {
            if (!it.second->IsProtected)
                it.second->Resume(fade);
        }
    }

    void SaXAudio::Protect(const INT32 voiceID)
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        AudioVoice* voice = GetVoice(voiceID);
        if (voice)
        {
            voice->IsProtected = true;
            if (voice->IsPlaying)
                while (voice->Resume());

            Log(voice->BankID, voiceID, "[Protect]");
        }
    }

    INT32 SaXAudio::Add(AudioData* data)
    {
        lock_guard<mutex> lock(m_bankMutex);

        Log(m_bankCounter, -1, "[Add]");

        m_bank[m_bankCounter++] = data;
        return m_bankCounter - 1;
    }

    void SaXAudio::Remove(const INT32 bankID)
    {
        lock_guard<mutex> lock(m_bankMutex);

        Log(bankID, -1, "[Remove]");

        // Delete all voices using that bankID
        for (auto& it : m_voices)
        {
            if (it.second->BankID == bankID)
                RemoveVoice(it.first);
        }

        auto it = m_bank.find(bankID);
        if (it != m_bank.end())
        {
            AudioData* data = it->second;
            // Free the audio buffer
            if (data->buffer)
            {
                delete data->buffer;
                data->buffer = nullptr;
            }

            // If the decoding is still happening we don't want to delete the data yet
            if (data->decodedSamples == data->totalSamples)
            {
                delete data;
                m_bank.erase(bankID);
            }
        }
    }

    INT32 SaXAudio::AddBus()
    {
        lock_guard<mutex> lock(m_busMutex);

        Log(-1, -1, "[AddBus]");

        IXAudio2SubmixVoice* bus;
        HRESULT hr = m_XAudio->CreateSubmixVoice(&bus, m_masterDetails.InputChannels, m_masterDetails.InputSampleRate);
        if (FAILED(hr))
        {
            return -1;
        }

        BusData data;
        data.voice = bus;
        data.effectChain = { 2, nullptr };
        data.descriptors[0] = { nullptr, false, SaXAudio::m_masterDetails.InputChannels };
        data.descriptors[1] = { nullptr, false, SaXAudio::m_masterDetails.InputChannels };

        m_buses[m_busCounter++] = data;
        return m_busCounter - 1;
    }

    void SaXAudio::RemoveBus(const INT32 busID)
    {
        lock_guard<mutex> lock(m_busMutex);

        Log(-1, -1, "[Remove] " + to_string(busID));

        // TODO: Delete all voices on that bus? Or do they get cleaned up automatically?

        auto it = m_buses.find(busID);
        if (it != m_buses.end())
        {
            it->second.voice->DestroyVoice();
            m_buses.erase(busID);
        }
    }

    BOOL SaXAudio::StartDecodeOgg(const INT32 bankID, const BYTE* buffer, const UINT32 length)
    {
        int error;
        stb_vorbis* vorbis = stb_vorbis_open_memory(buffer, length, &error, NULL);

        if (!vorbis)
            return FALSE;

        AudioData* data = nullptr;
        auto it = m_bank.find(bankID);
        if (it != m_bank.end())
            data = it->second;
        if (data)
        {
            // Get file info
            stb_vorbis_info info = stb_vorbis_get_info(vorbis);
            data->channels = info.channels;
            data->sampleRate = info.sample_rate;

            // Get total samples count and allocate the buffer
            data->totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
            data->buffer = new FLOAT[data->totalSamples * data->channels];

            thread decode(DecodeOgg, bankID, vorbis);
            decode.detach();
        }

        return data != nullptr;
    }

    AudioVoice* SaXAudio::CreateVoice(const INT32 bankID, const INT32 busID)
    {
        lock_guard<mutex> banklock(m_bankMutex);
        lock_guard<mutex> buslock(m_busMutex);
        lock_guard<mutex> voicelock(m_voiceMutex);

        AudioData* data = nullptr;

        auto it = m_bank.find(bankID);
        if (it != m_bank.end())
            data = it->second;

        if (!data)
        {
            return nullptr;
        }

        // Set up audio format
        WAVEFORMATEX wfx = { 0 };
        wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;  // 32-bit float format
        wfx.nChannels = static_cast<WORD>(data->channels);
        wfx.nSamplesPerSec = static_cast<DWORD>(data->sampleRate);
        wfx.wBitsPerSample = 32;  // 32-bit float
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.cbSize = 0;

        // Try to find a unused voice
        AudioVoice* voice = nullptr;
        for (auto& it : m_voices)
        {
            if (!it.second->BankData && !it.second->SourceVoice)
            {
                voice = it.second;
                m_voices.erase(it.first);
                break;
            }
        }

        // No unused voices, create one
        if (!voice)
        {
            voice = new AudioVoice;
        }

        IXAudio2SubmixVoice* bus = nullptr;
        if (busID >= 0)
        {
            auto it = m_buses.find(busID);
            if (it != m_buses.end())
                bus = it->second.voice;
        }

        HRESULT hr = 0;
        if (bus)
        {
            XAUDIO2_SEND_DESCRIPTOR sendDesc { 0, bus };
            XAUDIO2_VOICE_SENDS sends { 1, &sendDesc };

            HRESULT hr = m_XAudio->CreateSourceVoice(&voice->SourceVoice, &wfx, 0, XAUDIO2_MAX_FREQ_RATIO, voice, &sends, nullptr);
            if (FAILED(hr))
            {
                return nullptr;
            }
        }
        else
        {
            HRESULT hr = m_XAudio->CreateSourceVoice(&voice->SourceVoice, &wfx, 0, XAUDIO2_MAX_FREQ_RATIO, voice, nullptr, nullptr);
        }

        if (FAILED(hr))
        {
            return nullptr;
        }
        voice->BankData = data;

        // Submit audio buffer
        voice->Buffer = { 0 };
        voice->Buffer.AudioBytes = static_cast<UINT32>(sizeof(float) * data->totalSamples * data->channels);
        voice->Buffer.pAudioData = reinterpret_cast<const BYTE*>(data->buffer);
        voice->Buffer.Flags = XAUDIO2_END_OF_STREAM;

        voice->BankID = bankID;
        voice->VoiceID = m_voiceCounter++;

        // Set up the output matrix
        voice->SetOutputMatrix(0.0f);

        m_voices[voice->VoiceID] = voice;

        Log(bankID, voice->VoiceID, "[CreateVoice]" + (bus ? " Created on bus " + to_string(busID) : ""));

        return voice;
    }

    AudioVoice* SaXAudio::GetVoice(const INT32 voiceID)
    {
        AudioVoice* voice = nullptr;
        auto it = m_voices.find(voiceID);
        if (it != m_voices.end() && it->second->BankData)
            voice = it->second;
        return voice;
    }

    BusData* SaXAudio::GetBus(const INT32 busID)
    {
        BusData* bus = nullptr;
        auto it = m_buses.find(busID);
        if (it != m_buses.end())
            bus = &it->second;
        return bus;
    }


    void SaXAudio::CreateEffectChain(IXAudio2Voice* voice, EffectData* data)
    {
        HRESULT hr = XAudio2CreateReverb(&data->descriptors[1].pEffect);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to create reverb effect");
        }

        hr = CreateFX(__uuidof(FXEQ), &data->descriptors[0].pEffect);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to create Eq effect");
        }

        data->effectChain.EffectCount = 2;
        data->effectChain.pEffectDescriptors = data->descriptors;

        hr = voice->SetEffectChain(&data->effectChain);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to set effect chain");
        }
    }

    void SaXAudio::SetReverb(IXAudio2Voice* voice, EffectData* data)
    {
        if (!data->effectChain.pEffectDescriptors)
        {
            CreateEffectChain(voice, data);
        }

        HRESULT hr = voice->EnableEffect(1);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to enable reverb");
        }
        
        hr = voice->SetEffectParameters(1, &data->reverb, sizeof(XAUDIO2FX_REVERB_PARAMETERS), XAUDIO2_COMMIT_NOW);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to set effect parameters");
        }
    }

    void SaXAudio::RemoveReverb(IXAudio2Voice* voice, EffectData* data)
    {
        HRESULT hr = voice->DisableEffect(1);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to remove reverb");
        }
    }

    void SaXAudio::SetEq(IXAudio2Voice* voice, EffectData* data)
    {
        if (!data->effectChain.pEffectDescriptors)
        {
            CreateEffectChain(voice, data);
        }

        HRESULT hr = voice->EnableEffect(0);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to enable Eq");
        }

        hr = voice->SetEffectParameters(0, &data->eq, sizeof(FXEQ_PARAMETERS), XAUDIO2_COMMIT_NOW);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to set effect parameters");
        }
    }

    void SaXAudio::RemoveEq(IXAudio2Voice* voice, EffectData* data)
    {
        HRESULT hr = voice->DisableEffect(0);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to remove Eq");
        }
    }

    void SaXAudio::DecodeOgg(const INT32 bankID, stb_vorbis* vorbis)
    {
        // Reset file position
        stb_vorbis_seek_start(vorbis);

        UINT32 samplesTotal = 0;
        UINT32 samplesDecoded = 0;
        UINT32 bufferSize = 4096;

        auto it = m_bank.find(bankID);
        if (it != m_bank.end())
        {
            AudioData* data = it->second;
            if (data)
            {
                data->decodedSamples = 0;
                samplesTotal = data->totalSamples;
            }
        }

        while (samplesTotal - samplesDecoded > 0)
        {
            if (bufferSize > samplesTotal)
                bufferSize = samplesTotal;

            lock_guard<mutex> lock(m_bankMutex);

            AudioData* data = nullptr;
            it = m_bank.find(bankID);
            if (it != m_bank.end())
                data = it->second;

            if (data)
            {
                if (data->buffer)
                {
                    lock_guard<mutex> lock(data->decodingMutex);

                    // Read samples
                    FLOAT* pBuffer = &data->buffer[data->decodedSamples * data->channels];
                    UINT32 decoded = stb_vorbis_get_samples_float_interleaved(vorbis, data->channels, pBuffer, bufferSize * data->channels);
                    samplesDecoded += decoded;
                    data->decodedSamples = samplesDecoded;

                    if (decoded == 0)
                    {
                        // Less samples decoded than expected
                        // we update the total samples to match
                        data->totalSamples = samplesDecoded;
                    }

                    data->decodingPerform.notify_one();
                }
                else
                {
                    // Removed from the bank while still decoding
                    delete data;
                    m_bank.erase(bankID);
                    break;
                }
            }
            else
            {
                // Something is wrong
                samplesTotal = 0;
                Log(bankID, -1, "[DecodeOgg] Data is missing");
                break;
            }
        }

        // Close the Vorbis file
        stb_vorbis_close(vorbis);

        {
            lock_guard<mutex> lock(m_bankMutex);

            AudioData* data = nullptr;
            it = m_bank.find(bankID);
            if (it != m_bank.end())
                data = it->second;
            if (data) (*data->onDecodedCallback)(bankID);
        }

        Log(bankID, -1, "[DecodeOgg] Decoding complete");
    }

    void SaXAudio::RemoveVoice(const INT32 voiceID)
    {
        lock_guard<mutex> lock(m_voiceMutex);

        AudioVoice* voice = GetVoice(voiceID);
        if (voice)
        {
            if (voice->IsPlaying)
            {
                Log(voice->BankID, voiceID, "[RemoveVoice] Stopping voice");
                voice->Stop();
                // RemoveVoice will be called again by OnBufferEnd
            }
            else
            {
                voice->SourceVoice->DestroyVoice();
                voice->Reset();
                Log(voice->BankID, voiceID, "[RemoveVoice] Deleted voice");
            }
        }
    }
}
