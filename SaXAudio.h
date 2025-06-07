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
#include "Structs.h"
#include "AudioVoice.h"

namespace SaXAudio
{
    class SaXAudio
    {
        friend class AudioVoice;
    private:
        SaXAudio() = default;

        IXAudio2* m_XAudio = nullptr;
        IXAudio2MasteringVoice* m_masteringVoice = nullptr;

        unordered_map<INT32, BankData> m_bank;
        INT32 m_bankCounter = 0;
        mutex m_bankMutex;

        unordered_map<INT32, AudioVoice*> m_voices;
        INT32 m_voiceCounter = 0;
        mutex m_voiceMutex;

        unordered_map<INT32, BusData> m_buses;
        INT32 m_busCounter = 0;
        mutex m_busMutex;

        DWORD m_channelMask = 0;
        XAUDIO2_VOICE_DETAILS m_masterDetails = { 0 };

        static SaXAudio& getInstance()
        {
            static SaXAudio instance;
            return instance;
        }
        SaXAudio(const SaXAudio&) = delete;
        SaXAudio& operator=(const SaXAudio&) = delete;

    public:
        static SaXAudio& Instance;

        OnFinishedCallback OnFinishedCallback = nullptr;

        BOOL Init();

        void Release();

        void StopEngine();
        void StartEngine();

        void PauseAll(const FLOAT fade);
        void ResumeAll(const FLOAT fade);
        void Protect(const INT32 voiceID);

        BankData* AddBankEntry();
        void RemoveBankEntry(const INT32 bankID);

        INT32 AddBus();
        void RemoveBus(const INT32 busID);
        BusData* GetBus(const INT32 busID);

        BOOL StartDecodeOgg(const INT32 bankID, const BYTE* buffer, const UINT32 length);

        AudioVoice* CreateVoice(const INT32 bankID, const INT32 busID = -1);
        AudioVoice* GetVoice(const INT32 voiceID);

        void SetReverb(IXAudio2Voice* voice, EffectData* data);
        void RemoveReverb(IXAudio2Voice* voice, EffectData* data);

        void SetEq(IXAudio2Voice* voice, EffectData* data);
        void RemoveEq(IXAudio2Voice* voice, EffectData* data);

        void SetEcho(IXAudio2Voice* voice, EffectData* data);
        void RemoveEcho(IXAudio2Voice* voice, EffectData* data);

    private:
        static void DecodeOgg(const INT32 bankID, stb_vorbis* vorbis);
        void RemoveVoice(const INT32 voiceID);
        void CreateEffectChain(IXAudio2Voice* voice, EffectData* data);
    };
}
