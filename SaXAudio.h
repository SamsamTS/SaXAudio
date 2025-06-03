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
#include <xapofx.h>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
using namespace std;

#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

#define EXPORT extern "C" __declspec(dllexport)

namespace SaXAudio
{
    typedef void (*OnDecodedCallback)(INT32 bankID);
    typedef void (*OnFinishedCallback)(INT32 voiceID);


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
    /// Add/Modify the reverb effect to a voice or bus
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="reverbParams"></param>
    /// <param name="isBus"></param>
    EXPORT void SetReverb(const INT32 voiceID, const XAUDIO2FX_REVERB_PARAMETERS reverbParams, BOOL isBus = false);
    /// <summary>
    /// Remove the reverb effect to a voice or bus
    /// </summary>
    EXPORT void RemoveReverb(const INT32 voiceID, BOOL isBus = false);

    /// <summary>
    /// Add/Modify the EQ effect to a voice or bus
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="eqParams"></param>
    /// <param name="isBus"></param>
    EXPORT void SetEq(const INT32 voiceID, const FXEQ_PARAMETERS eqParams, BOOL isBus = false);
    /// <summary>
    /// Remove the EQ effect to a voice or bus
    /// </summary>
    EXPORT void RemoveEq(const INT32 voiceID, BOOL isBus = false);

    /// <summary>
    /// Add/Modify the echo effect to a voice or bus
    /// </summary>
    /// <param name="voiceID"></param>
    /// <param name="echoParams"></param>
    /// <param name="isBus"></param>
    EXPORT void SetEcho(const INT32 voiceID, const FXECHO_PARAMETERS echoParams, BOOL isBus = false);
    /// <summary>
    /// Remove the echo effect to a voice or bus
    /// </summary>
    EXPORT void RemoveEcho(const INT32 voiceID, BOOL isBus = false);

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


    struct EffectData
    {
        XAUDIO2_EFFECT_CHAIN effectChain = { 0 };
        XAUDIO2_EFFECT_DESCRIPTOR descriptors[3] = { 0 };
        XAUDIO2FX_REVERB_PARAMETERS reverb = { 0 };
        FXEQ_PARAMETERS eq = { 0 };
        FXECHO_PARAMETERS echo = { 0 };
    };

    struct BusData : EffectData
    {
        IXAudio2SubmixVoice* voice = nullptr;
    };

    struct AudioData
    {
        FLOAT* buffer = nullptr;
        UINT32 size = 0;
        OnDecodedCallback onDecodedCallback = nullptr;

        UINT32 channels = 0;
        UINT32 sampleRate = 0;
        UINT32 totalSamples = 0;

        atomic<UINT32> decodedSamples = 0;
        mutex decodingMutex;
        condition_variable decodingPerform;
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
        INT64 m_positionOffset = 0;
        FLOAT m_volumeTarget = 0;

        atomic<UINT32> m_tempFlush = 0;

    public:
        AudioData* BankData = nullptr;
        IXAudio2SourceVoice* SourceVoice = nullptr;
        XAUDIO2_BUFFER Buffer = { 0 };

        INT32 BankID = -1;
        INT32 VoiceID = -1;

        FLOAT Volume = 1.0f;
        FLOAT Speed = 1.0f;
        FLOAT Panning = 0.0f;

        EffectData EffectData;

        UINT32 LoopStart = 0;
        UINT32 LoopEnd = 0;

        atomic<BOOL> Looping = false;
        atomic<BOOL> IsPlaying = false;
        BOOL IsProtected = false;

        BOOL Start(const UINT32 atSample = 0, BOOL flush = true);
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
        static void WaitForDecoding(AudioVoice* voice);
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

        static unordered_map<INT32, BusData> m_buses;
        static INT32 m_busCounter;
        static mutex m_busMutex;

        static DWORD m_channelMask;
        static XAUDIO2_VOICE_DETAILS m_masterDetails;

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

        static INT32 AddBus();
        static void RemoveBus(const INT32 busID);
        static BusData* GetBus(const INT32 busID);

        static BOOL StartDecodeOgg(const INT32 bankID, const BYTE* buffer, const UINT32 length);

        static AudioVoice* CreateVoice(const INT32 bankID, const INT32 busID = -1);
        static AudioVoice* GetVoice(const INT32 voiceID);

        static void SetReverb(IXAudio2Voice* voice, EffectData* data);
        static void RemoveReverb(IXAudio2Voice* voice, EffectData* data);

        static void SetEq(IXAudio2Voice* voice, EffectData* data);
        static void RemoveEq(IXAudio2Voice* voice, EffectData* data);

        static void SetEcho(IXAudio2Voice* voice, EffectData* data);
        static void RemoveEcho(IXAudio2Voice* voice, EffectData* data);

    private:
        static void DecodeOgg(const INT32 bankID, stb_vorbis* vorbis);
        static void RemoveVoice(const INT32 voiceID);
        static void CreateEffectChain(IXAudio2Voice* voice, EffectData* data);
    };
}

#ifdef LOGGING

#include <fstream>
#include <iomanip>
#include <queue>

#define GetTime() chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()

struct LogEntry
{
    INT64 timestamp;
    INT32 bankID;
    INT32 voiceID;
    string message;
};

struct LogData
{
    atomic<bool> logging = false;

    mutex mutex;
    condition_variable condition;
    thread workThread;

    queue<LogEntry> queue;

    ofstream file;
    INT64 startTime = GetTime();
};

static LogData g_logData;

void LogWorker()
{
    g_logData.file.open("SaXAudio.log", ios::trunc | ios::out);

    vector<LogEntry> batch;
    batch.reserve(32);

    while (true)
    {
        // Wait for entries
        unique_lock<mutex> lock(g_logData.mutex);
        g_logData.condition.wait(lock, [] { return !g_logData.queue.empty() || !g_logData.logging; });

        // Move entries to local batch
        while (!g_logData.queue.empty())
        {
            batch.push_back(move(g_logData.queue.front()));
            g_logData.queue.pop();
        }
        lock.unlock();

        // Process batch
        for (const auto& entry : batch)
        {
            INT64 millisec = entry.timestamp;
            INT64 seconds = millisec / 1000;
            INT32 minutes = (INT32)(seconds / 60);
            INT32 hours = minutes / 60;

            g_logData.file << hours << ":"
                << setw(2) << setfill('0') << minutes % 60 << ":"
                << setw(2) << setfill('0') << seconds % 60 << "."
                << setw(3) << setfill('0') << millisec % 1000;

            if (entry.bankID >= 0)
                g_logData.file << " | " << setw(5) << setfill(' ') << left << ("B" + to_string(entry.bankID));
            else
                g_logData.file << " |      ";

            if (entry.voiceID >= 0)
                g_logData.file << " | " << setw(6) << setfill(' ') << left << ("V" + to_string(entry.voiceID)) << " | ";
            else
                g_logData.file << " |        | ";

            g_logData.file << entry.message << endl;
        }

        batch.clear();
        g_logData.file.flush();

        if (!g_logData.logging)
            break;
    }

    g_logData.file.close();
}

void StartLogging()
{
    if (!g_logData.workThread.joinable())
    {
        g_logData.logging = true;
        g_logData.workThread = thread(LogWorker);
    }
}

void StopLogging()
{
    g_logData.logging = false;
    g_logData.condition.notify_one();

    if (g_logData.workThread.joinable())
        g_logData.workThread.join();
}

void Log(const INT32 bankID, const INT32 voiceId, const string& message)
{
    if (!g_logData.logging)
        return;

    INT64 timestamp = GetTime() - g_logData.startTime;

    {
        lock_guard<mutex> lock(g_logData.mutex);
        g_logData.queue.push({ timestamp, bankID, voiceId, message });
    }

    g_logData.condition.notify_one();
}

#else
#define Log(bankID, voiceId, message)
#define StartLogging()
#define StopLogging()
#endif // LOGGING