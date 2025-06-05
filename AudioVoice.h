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

namespace SaXAudio
{
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
}
