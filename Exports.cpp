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
#include "Exports.h"

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
    EXPORT BOOL Create()
    {
        return SaXAudio::Instance.Init();
    }

    EXPORT void Release()
    {
        SaXAudio::Instance.Release();
    }

    EXPORT void StartEngine()
    {
        SaXAudio::Instance.StartEngine();
    }

    EXPORT void StopEngine()
    {
        SaXAudio::Instance.StopEngine();
    }

    EXPORT void PauseAll(const FLOAT fade)
    {
        SaXAudio::Instance.PauseAll(fade);
    }

    EXPORT void ResumeAll(const FLOAT fade)
    {
        SaXAudio::Instance.ResumeAll(fade);
    }

    EXPORT void Protect(const INT32 voiceID)
    {
        SaXAudio::Instance.Protect(voiceID);
    }

    EXPORT INT32 BankAddOgg(const BYTE* buffer, const UINT32 length, const OnDecodedCallback callback)
    {
        BankData* data = SaXAudio::Instance.AddBankEntry();
        if (!data) return -1;

        data->onDecodedCallback = callback;
        SaXAudio::Instance.StartDecodeOgg(data->bankID, buffer, length);
        return data->bankID;
    }

    EXPORT void BankRemove(const INT32 bankID)
    {
        SaXAudio::Instance.RemoveBankEntry(bankID);
    }

    EXPORT INT32 CreateVoice(const INT32 bankID, const INT32 busID, const BOOL paused)
    {
        AudioVoice* voice = SaXAudio::Instance.CreateVoice(bankID, busID);

        if (!voice)
            return -1;

        if (!paused)
            voice->Start();

        return voice->VoiceID;
    }

    EXPORT BOOL VoiceExist(const INT32 voiceID)
    {
        return SaXAudio::Instance.GetVoice(voiceID) != nullptr;
    }

    EXPORT INT32 CreateBus()
    {
        return SaXAudio::Instance.AddBus();
    }

    EXPORT void RemoveBus(INT32 busID)
    {
        SaXAudio::Instance.RemoveBus(busID);
    }

    EXPORT BOOL Start(const INT32 voiceID)
    {
        return StartAtSample(voiceID, 0);
    }

    EXPORT BOOL StartAtSample(const INT32 voiceID, const UINT32 sample)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Start(sample);
        }
        return false;
    }

    EXPORT BOOL StartAtTime(const INT32 voiceID, const FLOAT time)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice && voice->BankData && time >= 0)
        {
            UINT32 sample = (UINT32)(time * voice->BankData->sampleRate);
            return voice->Start(sample);
        }
        return false;
    }

    EXPORT BOOL Stop(const INT32 voiceID, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Stop(fade);
        }
        return false;
    }

    EXPORT UINT32 Pause(const INT32 voiceID, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Pause(fade);
        }
        return 0;
    }

    EXPORT UINT32 Resume(const INT32 voiceID, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Resume(fade);
        }
        return 0;
    }

    EXPORT UINT32 GetPauseStack(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->GetPauseStack();
        }
        return 0;
    }

    EXPORT void SetVolume(const INT32 voiceID, const FLOAT volume, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            voice->SetVolume(volume, fade);
        }
    }

    EXPORT void SetSpeed(const INT32 voiceID, const FLOAT speed, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            voice->SetSpeed(speed, fade);
        }
    }

    EXPORT void SetPanning(const INT32 voiceID, const FLOAT panning, const FLOAT fade)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            voice->SetPanning(panning, fade);
        }
    }

    EXPORT void SetLooping(const INT32 voiceID, const BOOL looping)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
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

    EXPORT void SetLoopPoints(const INT32 voiceID, const UINT32 start, const UINT32 end)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            voice->ChangeLoopPoints(start, end);
        }
    }

    EXPORT FLOAT GetVolume(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Volume;
        }
        return 1.0f;
    }

    EXPORT FLOAT GetSpeed(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Speed;
        }
        return 1.0f;
    }

    EXPORT FLOAT GetPanning(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Panning;
        }
        return 0.0f;
    }

    EXPORT BOOL GetLooping(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->Looping;
        }
        return false;
    }

    EXPORT void SetReverb(const INT32 voiceID, const XAUDIO2FX_REVERB_PARAMETERS reverbParams, const FLOAT fade, BOOL isBus)
    {
        SaXAudio::Instance.SetReverb(voiceID, isBus, &reverbParams, fade);
    }

    EXPORT void RemoveReverb(const INT32 voiceID, const FLOAT fade, BOOL isBus)
    {
        SaXAudio::Instance.RemoveReverb(voiceID, isBus, fade);
    }

    EXPORT void SetEq(const INT32 voiceID, const FXEQ_PARAMETERS eqParams, const FLOAT fade, BOOL isBus)
    {
        SaXAudio::Instance.SetEq(voiceID, isBus, &eqParams, fade);
    }

    EXPORT void RemoveEq(const INT32 voiceID, const FLOAT fade, BOOL isBus)
    {
        SaXAudio::Instance.RemoveEq(voiceID, isBus, fade);
    }

    EXPORT void SetEcho(const INT32 voiceID, const FXECHO_PARAMETERS echoParams, const FLOAT fade, BOOL isBus)
    {
        SaXAudio::Instance.SetEcho(voiceID, isBus, &echoParams, fade);
    }

    EXPORT void RemoveEcho(const INT32 voiceID, const FLOAT fade, BOOL isBus)
    {
        SaXAudio::Instance.RemoveEcho(voiceID, isBus, fade);
    }

    EXPORT UINT32 GetPositionSample(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice)
        {
            return voice->GetPosition();
        }
        return 0;
    }

    EXPORT FLOAT GetPositionTime(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return (FLOAT)voice->GetPosition() / voice->BankData->sampleRate;
        }
        return 0.0f;
    }

    EXPORT UINT32 GetTotalSample(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return voice->BankData->totalSamples;
        }
        return 0;
    }

    EXPORT FLOAT GetTotalTime(const INT32 voiceID)
    {
        AudioVoice* voice = SaXAudio::Instance.GetVoice(voiceID);
        if (voice && voice->BankData)
        {
            return (FLOAT)voice->BankData->totalSamples / voice->BankData->sampleRate;
        }
        return 0.0f;
    }

    EXPORT void SetOnFinishedCallback(const OnFinishedCallback callback)
    {
        Log(-1, -1, "[OnVoiceFinished]");
        SaXAudio::Instance.OnFinishedCallback = callback;
    }
}
