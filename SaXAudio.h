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

//#define WIN32_LEAN_AND_MEAN	// Exclude rarely-used stuff from Windows headers
//#include <windows.h>
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
#include <mutex>
#include <atomic>
using namespace std;

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define EXPORT extern "C" __declspec(dllexport)

namespace SaXAudio
{

    typedef void (*CallbackFunction)(INT32 voiceID);

    EXPORT BOOL Create();
    EXPORT void Release();

    EXPORT void StartEngine();
    EXPORT void StopEngine();

    EXPORT void PauseAll(FLOAT fade = 0.1f);
    EXPORT void ResumeAll(FLOAT fade = 0.1f);

    EXPORT INT32 BankAddOgg(const BYTE* buffer, UINT32 length);
    EXPORT void BankRemove(INT32 bankID);

    EXPORT INT32 CreateVoice(INT32 bankID, BOOL paused = true);
    EXPORT BOOL VoiceExist(INT32 voiceID);

    EXPORT BOOL Start(INT32 voiceID);
    EXPORT BOOL StartAtSample(INT32 voiceID, UINT32 sample);
    EXPORT BOOL StartAtTime(INT32 voiceID, FLOAT time);
    EXPORT BOOL Stop(INT32 voiceID, FLOAT fade = 0.1f);

    EXPORT UINT32 Pause(INT32 voiceID, FLOAT fade = 0.1f);
    EXPORT UINT32 Resume(INT32 voiceID, FLOAT fade = 0.1f);
    EXPORT UINT32 GetPauseStack(INT32 voiceID);

    EXPORT void SetVolume(INT32 voiceID, FLOAT volume, FLOAT fade = 0);
    EXPORT void SetSpeed(INT32 voiceID, FLOAT speed, FLOAT fade = 0);
    EXPORT void SetPanning(INT32 voiceID, FLOAT panning, FLOAT fade = 0);
    EXPORT void SetLooping(INT32 voiceID, BOOL looping);
    EXPORT void SetLoopPoints(INT32 voiceID, UINT32 start, UINT32 end);

    EXPORT FLOAT GetVolume(INT32 voiceID);
    EXPORT FLOAT GetSpeed(INT32 voiceID);
    EXPORT FLOAT GetPanning(INT32 voiceID);
    EXPORT BOOL GetLooping(INT32 voiceID);

    EXPORT UINT32 GetPositionSample(INT32 voiceID);
    EXPORT FLOAT GetPositionTime(INT32 voiceID);

    EXPORT void OnVoiceFinished(INT32 voiceID, CallbackFunction callback);

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

        BOOL Start(UINT32 atSample = 0);
        BOOL Stop(FLOAT fade = 0.0f);

        UINT32 Pause(FLOAT fade = 0.0f);
        UINT32 Resume(FLOAT fade = 0.0f);
        UINT32 GetPauseStack();

        UINT32 GetPosition();

        void ChangeLoopPoints(UINT32 start, UINT32 end);

        void SetVolume(FLOAT volume, FLOAT fade = 0);
        void SetSpeed(FLOAT speed, FLOAT fade = 0);
        void SetPanning(FLOAT panning, FLOAT fade = 0);

        void Reset();
        void SetOutputMatrix(FLOAT panning);

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
        static FLOAT GetRate(FLOAT from, FLOAT to, FLOAT duration);
        static void Fade(AudioVoice* voice);

        void FadeVolume(FLOAT to, FLOAT duration);
        void FadeSpeed(FLOAT to, FLOAT duration);
        void FadePanning(FLOAT to, FLOAT duration);
    };

    class SaXAudio
    {
        friend class AudioVoice;
    private:
        static IXAudio2* m_XAudio;
        static IXAudio2MasteringVoice* m_masteringVoice;

        static unordered_map<INT32, AudioData*> m_bank;
        static INT32 m_bankCounter;

        static unordered_map<INT32, AudioVoice*> m_voices;
        static INT32 m_voiceCounter;

        static DWORD m_channelMask;
        static UINT32 m_outputChannels;
    public:
        static BOOL Init();

        static void Release();

        static void StopEngine();
        static void StartEngine();

        static void PauseAll(FLOAT fade);
        static void ResumeAll(FLOAT fade);

        static INT32 Add(AudioData* data);
        static void Remove(INT32 bankID);

        static BOOL StartDecodeOgg(INT32 bankID, const BYTE* buffer, UINT32 length);

        static AudioVoice* CreateVoice(INT32 bankID);
        static AudioVoice* GetVoice(INT32 voiceID);
    private:
        static void DecodeOgg(INT32 bankID, stb_vorbis* vorbis);
        static void RemoveVoice(INT32 voiceID);
    };

    IXAudio2* SaXAudio::m_XAudio = nullptr;
    IXAudio2MasteringVoice* SaXAudio::m_masteringVoice = nullptr;

    unordered_map<INT32, AudioData*> SaXAudio::m_bank;
    INT32 SaXAudio::m_bankCounter = 0;

    unordered_map<INT32, AudioVoice*> SaXAudio::m_voices;
    INT32 SaXAudio::m_voiceCounter = 0;

    DWORD SaXAudio::m_channelMask = 0;
    UINT32 SaXAudio::m_outputChannels = 0;
}

#ifdef _DEBUG

#define GetTime() chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()
#include <chrono>
#include <fstream>
#include <iomanip>
mutex g_logMutex;
ofstream g_log;
INT64 g_startTime = GetTime();

void Log(INT32 bankID, INT32 voiceId, string message)
{
    g_logMutex.lock();

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
        g_log << " | " << setw(4) << setfill(' ') << left << "B" + to_string(bankID);
    else
        g_log << " |     ";

    if (voiceId > 0)
        g_log << " " << setw(5) << setfill(' ') << left << "V" + to_string(voiceId) << " | ";
    else
        g_log << "       | ";

    g_log << message << endl;
    g_log.flush();

    g_logMutex.unlock();
}

#else
#define Log(message)
#endif // DEBUG