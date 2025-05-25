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
    // AudioVoice class implementation
    // -------------------------------------------------------

    BOOL AudioVoice::Start(UINT32 atSample)
    {
        if (!SourceVoice || !BankData || IsPlaying) return false;
        Log(BankID, VoiceID, "[Start] at: " + to_string(atSample));

        if (atSample > 0)
            Buffer.PlayBegin = atSample;

        // Set up the looping
        if (Looping && LoopStart < (BankData->TotalSamples -1))
        {
            Buffer.LoopBegin = LoopStart;
            Buffer.LoopLength = (LoopEnd > 0) ? LoopEnd - LoopStart : (BankData->TotalSamples -1) - LoopStart;
            Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
            Log(BankID, VoiceID, "[Start] Loop: " + to_string(Buffer.LoopBegin) + " " + to_string(Buffer.LoopLength + Buffer.LoopBegin));
        }

        // Submitting the buffer
        if (FAILED(SourceVoice->SubmitSourceBuffer(&Buffer)))
        {
            return false;
        }

        IsPlaying = true;

        // Waiting for some decoded samples, don't want to read garbage
        INT32 timeout = 500; // 500ms timeout
        while (BankData->DecodedSamples == 0 && timeout > 0)
        {
            Sleep(10);
            timeout -= 10;
        }

        if (m_pauseStack > 0)
        {
            Log(BankID, VoiceID, "[Start] Voice paused");
            return IsPlaying;
        }

        Log(BankID, VoiceID, "[Start] Starting");
        if (timeout <= 0 || FAILED(SourceVoice->Start()))
        {
            Log(BankID, VoiceID, "[Start] FAILED starting" + (timeout <= 0) ? " - timeout" : "");
            IsPlaying = false;
        }

        return IsPlaying;
    }

    BOOL AudioVoice::Stop(FLOAT fade)
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
            SourceVoice->Stop();
            SourceVoice->FlushSourceBuffers();
        }

        return true;
    }

    UINT32 AudioVoice::Pause(FLOAT fade)
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

    UINT32 AudioVoice::Resume(FLOAT fade)
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
        UINT64 position = state.SamplesPlayed - m_positionReset + Buffer.PlayBegin;
        if (LoopEnd > LoopStart && position > LoopStart)
        {
            // We are somewhere in the loop
            position = LoopStart + ((position - LoopStart) % (LoopEnd - LoopStart));
        }

        if (position == 0)
            return 1; // Probably waiting on decoding, returning 0 would mean it finished playing

        return (INT32)position;
    }

    void AudioVoice::ChangeLoopPoints(UINT32 start, UINT32 end)
    {
        if (!SourceVoice || !BankData) return;
        Log(BankID, VoiceID, "[ChangeLoopPoints] start: " + to_string(start) + " end: " + to_string(end));

        if (end == 0)
            end = BankData->TotalSamples - 1;

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

        if (LoopStart == LoopEnd)
            LoopStart = LoopEnd - 1;

        if (!IsPlaying || !Looping)
        {
            return;
        }

        // We need to stop the voice to change the looping points
        SourceVoice->Stop();

        // We will calculate the current position in the buffer
        // TODO: make sure calculation is accurate at different speeds
        XAUDIO2_VOICE_STATE state;
        SourceVoice->GetState(&state);
        SourceVoice->FlushSourceBuffers();

        // We might not have started at from the beginning of the buffer so we need add PlayBegin
        UINT64 position = state.SamplesPlayed - m_positionReset + Buffer.PlayBegin;
        // Store the current samples played because SamplesPlayed cannot be reset to 0 making further calculations incorrect
        m_positionReset = state.SamplesPlayed;
        if (position > LoopStart)
        {
            // We are somewhere in the loop
            position = LoopStart + ((position - LoopStart) % (LoopEnd - LoopStart));
        }

        // TODO: Check if we are past the new looping points? How does XAudio handle it and what's the desired behavior?

        // Updating the buffer
        Buffer.PlayBegin = (UINT32)position;
        Buffer.LoopBegin = LoopStart;
        Buffer.LoopLength = (LoopEnd > 0) ? LoopEnd - LoopStart : Buffer.PlayLength - LoopStart;
        Buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
        if (SUCCEEDED(SourceVoice->SubmitSourceBuffer(&Buffer)))
        {
            // Resume playing
            SourceVoice->Start();
        }
    }

    void AudioVoice::SetVolume(FLOAT volume, FLOAT fade)
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

    void AudioVoice::SetSpeed(FLOAT speed, FLOAT fade)
    {
        // TODO: would this mess up GetPosition calculation?
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

    void AudioVoice::SetPanning(FLOAT panning, FLOAT fade)
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
    inline FLOAT FastCos(FLOAT x)
    {
        // Normalize to [0, π/2] and use Taylor series: cos(x) ≈ 1 - x²/2 + x⁴/24 - x⁶/720
        FLOAT x2 = x * x;
        FLOAT x4 = x2 * x2;
        FLOAT x6 = x4 * x2;
        return 1.0f - x2 * 0.5f + x4 * 0.041666667f - x6 * 0.001388889f;
    }

    // Fast approximation of sine using Taylor series (good for [0, π/2])
    inline FLOAT FastSin(FLOAT x)
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

        UINT32 sourceChannels = BankData->Channels;
        UINT32 destChannels = SaXAudio::m_outputChannels;
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
        m_positionReset = 0;
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

    inline FLOAT AudioVoice::GetRate(FLOAT from, FLOAT to, FLOAT duration)
    {
        Log(-1, -1, "[GetRate] from: " + to_string(from) + " to: " + to_string(to) + " duration: " + to_string(duration));
        return (to - from) / (duration * 100.0f);
    }

    inline FLOAT MoveToTarget(FLOAT start, FLOAT end, FLOAT rate)
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

        Log(voice->BankID, voice->VoiceID, "[Fade] Fading start" +
            (voice->m_fadeInfo.volumeRate != 0 ? " volumeRate: " + to_string(voice->m_fadeInfo.volumeRate) : "") +
            (voice->m_fadeInfo.speedRate != 0 ? " speedRate: " + to_string(voice->m_fadeInfo.speedRate) : "") +
            (voice->m_fadeInfo.panningRate != 0 ? " panningRate: " + to_string(voice->m_fadeInfo.panningRate) : ""));

#ifdef LOGGING
        auto now = GetTime();
#endif
        do
        {
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

            Sleep(10);
        }
        while (voice->m_fading && voice->SourceVoice && voice->BankData && (
            voice->m_fadeInfo.volumeRate != 0 ||
            (voice->m_fadeInfo.speedRate != 0 && !voice->m_fadeInfo.pause && !voice->m_fadeInfo.stop) ||
            (voice->m_fadeInfo.panningRate != 0 && !voice->m_fadeInfo.pause && !voice->m_fadeInfo.stop)));

        if (!voice->m_fading)
            return;

        if (voice->m_fadeInfo.speedRate != 0 && voice->m_fadeInfo.panningRate != 0)
        {
            Log(voice->BankID, voice->VoiceID, "[Fade] Fading complete V" + to_string(voice->VoiceID) + " in " + to_string(GetTime() - now));
        }

        if (voice->m_fadeInfo.pause)
        {
            Log(voice->BankID, voice->VoiceID, "[Fade] Pausing V" + to_string(voice->VoiceID));
            voice->SourceVoice->Stop();
            voice->m_fadeInfo.pause = false;
        }

        if (voice->m_fadeInfo.stop && voice->m_fadeInfo.volumeRate == 0)
        {
            Log(voice->BankID, voice->VoiceID, "[Fade] Stopping V" + to_string(voice->VoiceID));
            voice->SourceVoice->Stop();
            voice->SourceVoice->FlushSourceBuffers();
            voice->m_fadeInfo.stop = false;
        }
        voice->m_fading = false;
    }

    void AudioVoice::FadeVolume(FLOAT target, FLOAT duration)
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

    void AudioVoice::FadeSpeed(FLOAT target, FLOAT duration)
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

    void AudioVoice::FadePanning(FLOAT target, FLOAT duration)
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
        // We don't want to do anything when changing loop points
        if (Looping) return;
        Log(BankID, VoiceID, "[OnBufferEnd] V" + to_string(VoiceID));

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

    BOOL SaXAudio::Init()
    {
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

        // Count destination channels from channel mask
        m_outputChannels = 0;
        DWORD tempMask = m_channelMask;
        while (tempMask)
        {
            if (tempMask & 1) m_outputChannels++;
            tempMask >>= 1;
        }
        Log(-1, -1, "[Init] Initialization complete. Number of output channels: " + to_string(m_outputChannels));

        return true;
    }

    void SaXAudio::Release()
    {
        if (m_XAudio)
        {
            m_XAudio->Release();
        }
        // TODO clean bank
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

    void SaXAudio::PauseAll(FLOAT fade)
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        Log(-1, -1, "[PauseAll]");

        for (auto& it : m_voices)
        {
            it.second->Pause(fade);
        }
    }

    void SaXAudio::ResumeAll(FLOAT fade)
    {
        if (!m_XAudio || !m_masteringVoice)
            return;
        Log(-1, -1, "[ResumeAll]");

        for (auto& it : m_voices)
        {
            it.second->Resume(fade);
        }
    }

    INT32 SaXAudio::Add(AudioData* data)
    {
        Log(m_bankCounter, -1, "[Add]");

        // TODO thread safety?
        m_bank[m_bankCounter++] = data;
        return m_bankCounter - 1;
    }

    void SaXAudio::Remove(INT32 bankID)
    {
        Log(bankID, -1, "[Remove]");

        // Delete all voices using that bankID
        for (auto& it : m_voices)
        {
            if (it.second->BankID == bankID)
                RemoveVoice(it.first);
        }

        // TODO thread safety?
        auto it = m_bank.find(bankID);
        if (it != m_bank.end())
        {
            AudioData* data = it->second;
            // Free the audio buffer
            if (data->Buffer)
            {
                delete data->Buffer;
                data->Buffer = nullptr;
            }

            // If the decoding is still happening we don't want to delete the data yet
            if (data->DecodedSamples == data->TotalSamples)
            {
                delete data;
                m_bank.erase(bankID);
            }
        }

    }

    BOOL SaXAudio::StartDecodeOgg(INT32 bankID, const BYTE* buffer, UINT32 length)
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
            data->Channels = info.channels;
            data->SampleRate = info.sample_rate;

            // Get total samples count and allocate the buffer
            data->TotalSamples = stb_vorbis_stream_length_in_samples(vorbis);
            data->Buffer = new FLOAT[data->TotalSamples * data->Channels];

            thread decode(DecodeOgg, bankID, vorbis);
            decode.detach();
        }

        return data != nullptr;
    }

    AudioVoice* SaXAudio::CreateVoice(INT32 bankID)
    {
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
        wfx.nChannels = static_cast<WORD>(data->Channels);
        wfx.nSamplesPerSec = static_cast<DWORD>(data->SampleRate);
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

        HRESULT hr = m_XAudio->CreateSourceVoice(&voice->SourceVoice, &wfx, 0, XAUDIO2_MAX_FREQ_RATIO, voice, nullptr, nullptr);
        if (FAILED(hr))
        {
            return nullptr;
        }
        voice->BankData = data;

        // Submit audio buffer
        voice->Buffer = { 0 };
        voice->Buffer.AudioBytes = static_cast<UINT32>(sizeof(float) * data->TotalSamples * data->Channels);
        voice->Buffer.pAudioData = reinterpret_cast<const BYTE*>(data->Buffer);
        voice->Buffer.Flags = XAUDIO2_END_OF_STREAM;

        voice->BankID = bankID;
        voice->VoiceID = m_voiceCounter++;

        // Set up the output matrix
        voice->SetOutputMatrix(0.0f);

        m_voices[voice->VoiceID] = voice;

        Log(bankID, voice->VoiceID, "[CreateVoice] Created voice");

        return voice;
    }

    AudioVoice* SaXAudio::GetVoice(INT32 voiceID)
    {
        AudioVoice* voice = nullptr;
        auto it = m_voices.find(voiceID);
        if (it != m_voices.end() && it->second->BankData)
            voice = it->second;
        return voice;
    }

    void SaXAudio::DecodeOgg(INT32 bankID, stb_vorbis* vorbis)
    {
        // Reset file position
        stb_vorbis_seek_start(vorbis);

        UINT32 samplesTotal = 0;
        UINT32 samplesDecoded = 0;
        UINT32 bufferSize = 4096;  // Read 4096 samples at a time

        auto it = m_bank.find(bankID);
        if (it != m_bank.end())
        {
            AudioData* data = it->second;
            if (data)
            {
                data->DecodedSamples = 0;
                samplesTotal = data->TotalSamples;
            }
        }

        Log(bankID, -1, "[DecodeOgg] Decoding");

        while (samplesTotal - samplesDecoded > 0)
        {
            if (bufferSize > samplesTotal)
                bufferSize = samplesTotal;

            AudioData* data = nullptr;
            it = m_bank.find(bankID);
            if (it != m_bank.end())
                data = it->second;

            if (!data)
            {
                // Something is wrong
                samplesTotal = 0;
            }
            else if (data->Buffer)
            {
                // Read samples
                FLOAT* pBuffer = &data->Buffer[data->DecodedSamples * data->Channels];
                UINT32 decoded = stb_vorbis_get_samples_float_interleaved(vorbis, data->Channels, pBuffer, bufferSize * data->Channels);
                samplesDecoded += decoded;
                data->DecodedSamples = samplesDecoded;

                if (decoded == 0)
                {
                    // Less samples decoded than expected
                    // we update the total samples to match
                    data->TotalSamples = samplesDecoded;
                }
            }
            else
            {
                // Removed from the bank while still decoding
                delete data;
                m_bank[bankID] = nullptr;
                samplesDecoded = 0;
            }

        }

        // Close the Vorbis file
        stb_vorbis_close(vorbis);

        Log(bankID, -1, "[DecodeOgg] Decoding complete");
    }

    void SaXAudio::RemoveVoice(INT32 voiceID)
    {
        AudioVoice* voice = GetVoice(voiceID);
        if (voice)
        {
            if (voice->IsPlaying)
            {
                Log(voice->BankID, voiceID, "[RemoveVoice] Stopping voice");
                voice->Looping = false;
                voice->SourceVoice->Stop();
                voice->SourceVoice->FlushSourceBuffers();
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
    EXPORT void PauseAll(FLOAT fade)
    {
        SaXAudio::PauseAll(fade);
    }
    /// <summary>
    /// Resume playing all voices
    /// </summary>
    EXPORT void ResumeAll(FLOAT fade)
    {
        SaXAudio::ResumeAll(fade);
    }

    /// <summary>
    /// Add ogg audio data to the sound bank
    /// The data will be decoded (async) and stored in memory
    /// </summary>
    /// <param name="buffer">The ogg data</param>
    /// <param name="length">The length in bytes of the data</param>
    /// <returns>unique bankID for that audio data</returns>
    EXPORT INT32 BankAddOgg(const BYTE* buffer, UINT32 length)
    {
        AudioData* data = new AudioData;
        INT32 bankID = SaXAudio::Add(data);
        if (bankID >= 0)
            SaXAudio::StartDecodeOgg(bankID, buffer, length);
        return bankID;
    }
    /// <summary>
    /// Remove and free the memory of the specified audio data
    /// </summary>
    /// <param name="bankID">The bankID of the data to remove</param>
    EXPORT void BankRemove(INT32 bankID)
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
    EXPORT INT32 CreateVoice(INT32 bankID, BOOL paused)
    {
        AudioVoice* voice = SaXAudio::CreateVoice(bankID);

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
    EXPORT BOOL VoiceExist(INT32 voiceID)
    {
        return SaXAudio::GetVoice(voiceID) != nullptr;
    }

    /// <summary>
    /// Starts playing the specified voice
    /// Resets the pause stack
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>true if successful or already playing</returns>
    EXPORT BOOL Start(INT32 voiceID)
    {
        return StartAtSample(voiceID, 0);
    }

    /// <summary>
    /// Starts playing the specified voice at a specific sample
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="sample"></param>
    /// <returns>true if successful</returns>
    EXPORT BOOL StartAtSample(INT32 voiceID, UINT32 sample)
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
    EXPORT BOOL StartAtTime(INT32 voiceID, FLOAT time)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice && voice->BankData && time >= 0)
        {
            UINT32 sample = (UINT32)(time * voice->BankData->SampleRate);
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
    EXPORT BOOL Stop(INT32 voiceID, FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Stop(fade);
        }
        return false;
    }

    /// <summary>
    /// Pause the voice and increase the pause stack
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="fade">The duration of the fade in seconds</param>
    /// <returns>value of the pause stack</returns>
    EXPORT UINT32 Pause(INT32 voiceID, FLOAT fade)
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
    EXPORT UINT32 Resume(INT32 voiceID, FLOAT fade)
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
    EXPORT UINT32 GetPauseStack(INT32 voiceID)
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
    EXPORT void SetVolume(INT32 voiceID, FLOAT volume, FLOAT fade)
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
    EXPORT void SetSpeed(INT32 voiceID, FLOAT speed, FLOAT fade)
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
    EXPORT void SetPanning(INT32 voiceID, FLOAT panning, FLOAT fade)
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
    EXPORT void SetLooping(INT32 voiceID, BOOL looping)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            if (looping == voice->Looping)
                return;

            voice->Looping = looping;
            if (looping)
            {
                // Start looping
                if (voice->IsPlaying)
                {
                    voice->ChangeLoopPoints(voice->LoopStart, voice->LoopEnd);
                }
            }
            else if (voice->IsPlaying && voice->SourceVoice)
            {
                // Stop looping
                // TODO: this should be done by sending a new source buffer so the audio can continue playing past LoopEnd
                voice->SourceVoice->ExitLoop();
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
    EXPORT void SetLoopPoints(INT32 voiceID, UINT32 start, UINT32 end)
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
    EXPORT FLOAT GetVolume(INT32 voiceID)
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
    EXPORT FLOAT GetSpeed(INT32 voiceID)
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
    EXPORT FLOAT GetPanning(INT32 voiceID)
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
    EXPORT BOOL GetLooping(INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice)
        {
            return voice->Looping;
        }
        return false;
    }

    /// <summary>
    /// Gets the position of the playing voice in samples
    /// </summary>
    /// <param name="voiceID"></param>
    /// <returns>sample position if playing or 0</returns>
    EXPORT UINT32 GetPositionSample(INT32 voiceID)
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
    EXPORT FLOAT GetPositionTime(INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return (FLOAT)voice->GetPosition() / voice->BankData->SampleRate;
        }
        return 0.0f;
    }

    /// <summary>
    /// Sets a callback for when the voice finished playing
    /// </summary>
    /// <param name="callback"></param>
    EXPORT void SetOnFinishedCallback(OnFinishedCallback callback)
    {
        Log(-1, -1, "[OnVoiceFinished]");
        SaXAudio::OnFinishedCallback = callback;
    }
}
