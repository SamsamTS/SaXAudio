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

namespace SaXAudio
{
    SaXAudio& SaXAudio::Instance = SaXAudio::getInstance();

    BOOL SaXAudio::Init()
    {
        if (m_masteringVoice)
            return true;

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
        hr = XAudio2Create(&m_XAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
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
        m_masteringVoice = nullptr;
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
        data.effectChain = { 3, nullptr };
        data.descriptors[0] = { nullptr, false, SaXAudio::m_masterDetails.InputChannels };
        data.descriptors[1] = { nullptr, false, SaXAudio::m_masterDetails.InputChannels };
        data.descriptors[2] = { nullptr, false, SaXAudio::m_masterDetails.InputChannels };

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
            Log(-1, -1, "Failed to set reverb parameters");
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
            Log(-1, -1, "Failed to enable EQ");
        }

        hr = voice->SetEffectParameters(0, &data->eq, sizeof(FXEQ_PARAMETERS), XAUDIO2_COMMIT_NOW);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to set EQ effect parameters");
        }
    }

    void SaXAudio::RemoveEq(IXAudio2Voice* voice, EffectData* data)
    {
        HRESULT hr = voice->DisableEffect(0);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to remove EQ");
        }
    }

    void SaXAudio::SetEcho(IXAudio2Voice* voice, EffectData* data)
    {
        if (!data->effectChain.pEffectDescriptors)
        {
            CreateEffectChain(voice, data);
        }

        HRESULT hr = voice->EnableEffect(2);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to enable echo");
        }

        hr = voice->SetEffectParameters(0, &data->echo, sizeof(FXECHO_PARAMETERS), XAUDIO2_COMMIT_NOW);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to set echo parameters");
        }
    }

    void SaXAudio::RemoveEcho(IXAudio2Voice* voice, EffectData* data)
    {
        HRESULT hr = voice->DisableEffect(2);
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

        auto it = SaXAudio::Instance.m_bank.find(bankID);
        if (it != SaXAudio::Instance.m_bank.end())
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

            lock_guard<mutex> lock(SaXAudio::Instance.m_bankMutex);

            AudioData* data = nullptr;
            it = SaXAudio::Instance.m_bank.find(bankID);
            if (it != SaXAudio::Instance.m_bank.end())
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
                    SaXAudio::Instance.m_bank.erase(bankID);
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
            // Calling back
            lock_guard<mutex> lock(SaXAudio::Instance.m_bankMutex);

            AudioData* data = nullptr;
            it = SaXAudio::Instance.m_bank.find(bankID);
            if (it != SaXAudio::Instance.m_bank.end())
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
            Log(-1, -1, "Failed to create EQ effect");
        }

        hr = CreateFX(__uuidof(FXEcho), &data->descriptors[2].pEffect);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to create echo effect");
        }

        data->effectChain.EffectCount = 3;
        data->effectChain.pEffectDescriptors = data->descriptors;

        hr = voice->SetEffectChain(&data->effectChain);
        if (FAILED(hr))
        {
            Log(-1, -1, "Failed to set effect chain");
        }
    }
}
