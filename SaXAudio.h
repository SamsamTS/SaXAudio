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

//---------------------------------------------------------------------
// Enables logging
//---------------------------------------------------------------------
#define LOGGING

// Include minimal required stuff from windows
#ifdef _M_X64
#define _AMD64_
#elif defined _M_IX86
#define _X86_
#endif

#include <minwindef.h>
#include <winnt.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
using namespace std;

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define EXPORT extern "C" __declspec(dllexport)

namespace SaXAudio
{
    typedef void (*OnFinishedCallback)(INT32 voiceID);

    EXPORT BOOL Create();
    EXPORT void Release();

    EXPORT void StartEngine();
    EXPORT void StopEngine();

    EXPORT void PauseAll(const FLOAT fade = 0.1f);
    EXPORT void ResumeAll(const FLOAT fade = 0.1f);
    EXPORT void Protect(const INT32 voiceID);

    EXPORT INT32 BankAddOgg(const BYTE* buffer, const UINT32 length);
    EXPORT void BankRemove(const INT32 bankID);

    EXPORT INT32 CreateVoice(const INT32 bankID, const BOOL paused = true);
    EXPORT BOOL VoiceExist(const INT32 voiceID);

    EXPORT BOOL Start(const INT32 voiceID);
    EXPORT BOOL StartAtSample(const INT32 voiceID, const UINT32 sample);
    EXPORT BOOL StartAtTime(const INT32 voiceID, const FLOAT time);
    EXPORT BOOL Stop(const INT32 voiceID, const FLOAT fade = 0.1f);

    EXPORT UINT32 Pause(const INT32 voiceID, const FLOAT fade = 0.1f);
    EXPORT UINT32 Resume(const INT32 voiceID, const FLOAT fade = 0.1f);
    EXPORT UINT32 GetPauseStack(const INT32 voiceID);

    EXPORT void SetVolume(const INT32 voiceID, const FLOAT volume, const FLOAT fade = 0);
    EXPORT void SetSpeed(const INT32 voiceID, const FLOAT speed, const FLOAT fade = 0);
    EXPORT void SetPanning(const INT32 voiceID, const FLOAT panning, const FLOAT fade = 0);
    EXPORT void SetLooping(const INT32 voiceID, const BOOL looping);
    EXPORT void SetLoopPoints(const INT32 voiceID, const UINT32 start, const UINT32 end);

    EXPORT FLOAT GetVolume(const INT32 voiceID);
    EXPORT FLOAT GetSpeed(const INT32 voiceID);
    EXPORT FLOAT GetPanning(const INT32 voiceID);
    EXPORT BOOL GetLooping(const INT32 voiceID);

    EXPORT UINT32 GetPositionSample(const INT32 voiceID);
    EXPORT FLOAT GetPositionTime(const INT32 voiceID);

    EXPORT void SetOnFinishedCallback(const OnFinishedCallback callback);

    struct AudioData
    {
        FLOAT* Buffer = nullptr;
        UINT32 Size = 0;

        UINT32 Channels = 0;
        UINT32 SampleRate = 0;
        UINT32 TotalSamples = 0;

        atomic<UINT32> DecodedSamples = 0;
    };

    class AudioVoice : public IXAudio2VoiceCallback
    {
    private:
        struct FadeInfo
        {
            FLOAT volume = 0;
            FLOAT volumeTarget = 0;
            FLOAT volumeRate = 0;

            FLOAT speed = 0;
            FLOAT speedTarget = 0;
            FLOAT speedRate = 0;

            FLOAT panning = 0;
            FLOAT panningTarget = 0;
            FLOAT panningRate = 0;

            BOOL pause = false;
            BOOL stop = false;
        } m_fadeInfo = {};
        atomic<BOOL> m_fading = false;

        atomic<UINT32> m_pauseStack = 0;
        UINT64 m_positionReset = 0;
        FLOAT m_volumeTarget = 0;

        atomic<BOOL> m_tempFlush = false;

    public:
        AudioData* BankData = nullptr;
        IXAudio2SourceVoice* SourceVoice = nullptr;
        XAUDIO2_BUFFER Buffer = { 0 };

        INT32 BankID = -1;
        INT32 VoiceID = -1;

        FLOAT Volume = 1.0f;
        FLOAT Speed = 1.0f;
        FLOAT Panning = 0.0f;

        UINT32 LoopStart = 0;
        UINT32 LoopEnd = 0;

        atomic<BOOL> Looping = false;
        atomic<BOOL> IsPlaying = false;
        BOOL IsProtected = false;

        BOOL Start(const UINT32 atSample = 0);
        BOOL Stop(const FLOAT fade = 0.0f);

        UINT32 Pause(const FLOAT fade = 0.0f);
        UINT32 Resume(const FLOAT fade = 0.0f);
        UINT32 GetPauseStack();

        UINT32 GetPosition();

        void ChangeLoopPoints(const UINT32 start, const UINT32 end);

        void SetVolume(const FLOAT volume, const FLOAT fade = 0);
        void SetSpeed(FLOAT speed, const FLOAT fade = 0);
        void SetPanning(FLOAT panning, const FLOAT fade = 0);

        void Reset();
        void SetOutputMatrix(const FLOAT panning);

        // Callbacks
        void __stdcall OnBufferEnd(void* pBufferContext) override;

        // Required methods (not used)
        void __stdcall OnVoiceProcessingPassStart(UINT32) override {}
        void __stdcall OnVoiceProcessingPassEnd() override {}
        void __stdcall OnStreamEnd() override {}
        void __stdcall OnBufferStart(void*) override { IsPlaying = true; }
        void __stdcall OnLoopEnd(void*) override {}
        void __stdcall OnVoiceError(void*, HRESULT) override {}

    private:
        static FLOAT GetRate(const FLOAT from, const FLOAT to, const FLOAT duration);
        static void Fade(AudioVoice* voice);

        void FadeVolume(const FLOAT to, const FLOAT duration);
        void FadeSpeed(const FLOAT to, const FLOAT duration);
        void FadePanning(const FLOAT to, const FLOAT duration);
    };

    class SaXAudio
    {
        friend class AudioVoice;
    private:
        static IXAudio2* m_XAudio;
        static IXAudio2MasteringVoice* m_masteringVoice;

        static unordered_map<INT32, AudioData*> m_bank;
        static INT32 m_bankCounter;
        static mutex m_bankMutex;

        static unordered_map<INT32, AudioVoice*> m_voices;
        static INT32 m_voiceCounter;
        static mutex m_voiceMutex;

        static DWORD m_channelMask;
        static UINT32 m_outputChannels;

    public:
        static OnFinishedCallback OnFinishedCallback;

        static BOOL Init();

        static void Release();

        static void StopEngine();
        static void StartEngine();

        static void PauseAll(const FLOAT fade);
        static void ResumeAll(const FLOAT fade);
        static void Protect(const INT32 voiceID);

        static INT32 Add(AudioData* data);
        static void Remove(const INT32 bankID);

        static BOOL StartDecodeOgg(const INT32 bankID, const BYTE* buffer, const UINT32 length);

        static AudioVoice* CreateVoice(const INT32 bankID);
        static AudioVoice* GetVoice(const INT32 voiceID);

    private:
        static void DecodeOgg(const INT32 bankID, stb_vorbis* vorbis);
        static void RemoveVoice(const INT32 voiceID);
    };
}

#ifdef LOGGING

#define GetTime() chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()
#include <chrono>
#include <fstream>
#include <iomanip>
mutex g_logMutex;
ofstream g_log;
INT64 g_startTime = GetTime();

void Log(const INT32 bankID, const INT32 voiceId, const string& message)
{
    lock_guard<mutex> lock(g_logMutex);

    if (!g_log.is_open())
        g_log.open("SaXAudio.log", fstream::trunc | fstream::out);

    INT64 millisec = GetTime() - g_startTime;
    INT64 seconds = millisec / 1000;
    INT32 minutes = (INT32)(seconds / 60);
    INT32 hours = minutes / 60;

    g_log << hours << ":"
        << setw(2) << setfill('0') << minutes % 60 << ":"
        << setw(2) << setfill('0') << seconds % 60 << "."
        << setw(3) << setfill('0') << millisec % 1000;

    if (bankID > 0)
        g_log << " | " << setw(5) << setfill(' ') << left << "B" + to_string(bankID);
    else
        g_log << " |      ";

    if (voiceId > 0)
        g_log << " | " << setw(6) << setfill(' ') << left << "V" + to_string(voiceId) << " | ";
    else
        g_log << " |        | ";

    g_log << message << endl;
    g_log.flush();
}

#else
#define Log(bankID, voiceId, message)
#endif // LOGGING