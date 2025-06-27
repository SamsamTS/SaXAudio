#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Exports.h"
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

using namespace testing;
using namespace SaXAudio;

class SaXAudioTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize the audio system before each test
        ASSERT_TRUE(Create()) << "Failed to initialize SaXAudio";
    }

    void TearDown() override {
        // Clean up after each test
        Release();
    }

    // Helper method to create a dummy OGG buffer for testing
    std::vector<BYTE> CreateDummyOggBuffer() {
        // This would normally contain valid OGG data
        // For testing purposes, create a minimal buffer
        std::vector<BYTE> buffer(1024, 0);
        // Add some dummy OGG header-like data
        buffer[0] = 'O'; buffer[1] = 'g'; buffer[2] = 'g'; buffer[3] = 'S';
        return buffer;
    }
};

// Basic initialization and cleanup tests
TEST_F(SaXAudioTest, InitializationAndCleanup) {
    // Test is handled by SetUp/TearDown
    SUCCEED();
}

TEST_F(SaXAudioTest, MultipleInitialization) {
    // Test multiple Create() calls
    EXPECT_TRUE(Create()); // Should handle multiple calls gracefully
    EXPECT_TRUE(Create());
}

// Engine control tests
TEST_F(SaXAudioTest, EngineControl) {
    // Test engine start/stop
    EXPECT_NO_THROW(StartEngine());
    EXPECT_NO_THROW(StopEngine());
    EXPECT_NO_THROW(StartEngine());
}

// Bank management tests
TEST_F(SaXAudioTest, BankManagement) {
    auto oggBuffer = CreateDummyOggBuffer();
    
    // Add audio data to bank
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    EXPECT_GT(bankID, 0) << "BankAddOgg should return positive ID";
    
    // Remove from bank
    EXPECT_NO_THROW(BankRemove(bankID));
}

TEST_F(SaXAudioTest, BankWithCallback) {
    auto oggBuffer = CreateDummyOggBuffer();
    bool callbackCalled = false;
    
    OnDecodedCallback callback = [&callbackCalled]() {
        callbackCalled = true;
    };
    
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()), callback);
    EXPECT_GT(bankID, 0);
    
    // Wait a bit for async decoding
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    BankRemove(bankID);
    // Note: Callback timing depends on implementation
}

TEST_F(SaXAudioTest, InvalidBankOperations) {
    // Test with invalid bank ID
    EXPECT_NO_THROW(BankRemove(-1));
    EXPECT_NO_THROW(BankRemove(99999));
}

// Bus management tests
TEST_F(SaXAudioTest, BusManagement) {
    INT32 busID = CreateBus();
    EXPECT_GT(busID, 0) << "CreateBus should return positive ID";
    
    EXPECT_NO_THROW(RemoveBus(busID));
}

TEST_F(SaXAudioTest, MultipleBuses) {
    std::vector<INT32> busIDs;
    
    // Create multiple buses
    for (int i = 0; i < 5; ++i) {
        INT32 busID = CreateBus();
        EXPECT_GT(busID, 0);
        busIDs.push_back(busID);
    }
    
    // Remove all buses
    for (INT32 busID : busIDs) {
        EXPECT_NO_THROW(RemoveBus(busID));
    }
}

// Voice management tests
TEST_F(SaXAudioTest, VoiceCreation) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    
    // Create voice
    INT32 voiceID = CreateVoice(bankID, busID, true);
    EXPECT_GT(voiceID, 0) << "CreateVoice should return positive ID";
    
    // Check if voice exists
    EXPECT_TRUE(VoiceExist(voiceID));
    
    // Cleanup
    EXPECT_NO_THROW(Stop(voiceID));
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, VoiceExistence) {
    // Test non-existent voice
    EXPECT_FALSE(VoiceExist(-1));
    EXPECT_FALSE(VoiceExist(99999));
}

// Playback control tests
TEST_F(SaXAudioTest, PlaybackControl) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test start/stop
    EXPECT_TRUE(Start(voiceID));
    EXPECT_TRUE(Stop(voiceID));
    
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, PlaybackAtSpecificPositions) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test starting at specific sample
    EXPECT_TRUE(StartAtSample(voiceID, 100));
    Stop(voiceID);
    
    // Test starting at specific time
    voiceID = CreateVoice(bankID, busID, true); // Create new voice
    EXPECT_TRUE(StartAtTime(voiceID, 1.0f));
    Stop(voiceID);
    
    BankRemove(bankID);
    RemoveBus(busID);
}

// Pause/Resume tests
TEST_F(SaXAudioTest, PauseResumeStack) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, false);
    
    // Test pause stack
    UINT32 pauseCount1 = Pause(voiceID);
    EXPECT_EQ(pauseCount1, 1);
    
    UINT32 pauseCount2 = Pause(voiceID);
    EXPECT_EQ(pauseCount2, 2);
    
    EXPECT_EQ(GetPauseStack(voiceID), 2);
    
    // Resume once
    UINT32 resumeCount1 = Resume(voiceID);
    EXPECT_EQ(resumeCount1, 1);
    
    // Resume again
    UINT32 resumeCount2 = Resume(voiceID);
    EXPECT_EQ(resumeCount2, 0);
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, GlobalPauseResume) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, false);
    
    // Test global pause/resume
    EXPECT_NO_THROW(PauseAll());
    EXPECT_NO_THROW(ResumeAll());
    
    // Test with custom fade
    EXPECT_NO_THROW(PauseAll(0.5f));
    EXPECT_NO_THROW(ResumeAll(0.5f));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, VoiceProtection) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, false);
    
    // Protect voice
    EXPECT_NO_THROW(Protect(voiceID));
    
    // Protected voice should not be affected by PauseAll/ResumeAll
    EXPECT_NO_THROW(PauseAll());
    // Voice should still be playing (implementation-dependent verification)
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

// Audio properties tests
TEST_F(SaXAudioTest, VolumeControl) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test volume setting/getting
    EXPECT_NO_THROW(SetVolume(voiceID, 0.5f));
    EXPECT_FLOAT_EQ(GetVolume(voiceID), 0.5f);
    
    // Test with fade
    EXPECT_NO_THROW(SetVolume(voiceID, 0.8f, 0.2f));
    
    // Test edge cases
    EXPECT_NO_THROW(SetVolume(voiceID, 0.0f));
    EXPECT_NO_THROW(SetVolume(voiceID, 1.0f));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, SpeedControl) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test speed setting/getting
    EXPECT_NO_THROW(SetSpeed(voiceID, 1.5f));
    EXPECT_FLOAT_EQ(GetSpeed(voiceID), 1.5f);
    
    // Test with fade
    EXPECT_NO_THROW(SetSpeed(voiceID, 0.8f, 0.1f));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, PanningControl) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test panning setting/getting
    EXPECT_NO_THROW(SetPanning(voiceID, -0.5f));
    EXPECT_FLOAT_EQ(GetPanning(voiceID), -0.5f);
    
    // Test edge cases
    EXPECT_NO_THROW(SetPanning(voiceID, -1.0f));
    EXPECT_NO_THROW(SetPanning(voiceID, 1.0f));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

// Looping tests
TEST_F(SaXAudioTest, LoopingControl) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test looping setting/getting
    EXPECT_NO_THROW(SetLooping(voiceID, true));
    EXPECT_TRUE(GetLooping(voiceID));
    
    EXPECT_NO_THROW(SetLooping(voiceID, false));
    EXPECT_FALSE(GetLooping(voiceID));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, LoopPoints) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test loop points setting/getting
    EXPECT_NO_THROW(SetLoopPoints(voiceID, 100, 500));
    EXPECT_EQ(GetLoopStart(voiceID), 100);
    EXPECT_EQ(GetLoopEnd(voiceID), 500);
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

// Effects tests
TEST_F(SaXAudioTest, ReverbEffect) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Create reverb parameters
    XAUDIO2FX_REVERB_PARAMETERS reverbParams = {};
    reverbParams.WetDryMix = 50.0f;
    reverbParams.ReflectionsDelay = 10;
    reverbParams.ReverbDelay = 40;
    reverbParams.RearDelay = 20;
    
    // Test reverb setting/removal
    EXPECT_NO_THROW(SetReverb(voiceID, reverbParams));
    EXPECT_NO_THROW(RemoveReverb(voiceID));
    
    // Test with fade
    EXPECT_NO_THROW(SetReverb(voiceID, reverbParams, 0.1f));
    EXPECT_NO_THROW(RemoveReverb(voiceID, 0.1f));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, EQEffect) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Create EQ parameters
    FXEQ_PARAMETERS eqParams = {};
    eqParams.FrequencyCenter0 = 100.0f;
    eqParams.Gain0 = 1.0f;
    eqParams.Bandwidth0 = 1.0f;
    
    // Test EQ setting/removal
    EXPECT_NO_THROW(SetEq(voiceID, eqParams));
    EXPECT_NO_THROW(RemoveEq(voiceID));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

TEST_F(SaXAudioTest, EchoEffect) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Create echo parameters
    FXECHO_PARAMETERS echoParams = {};
    echoParams.WetDryMix = 50.0f;
    echoParams.Feedback = 0.5f;
    echoParams.Delay = 500.0f;
    
    // Test echo setting/removal
    EXPECT_NO_THROW(SetEcho(voiceID, echoParams));
    EXPECT_NO_THROW(RemoveEcho(voiceID));
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

// Position and timing tests
TEST_F(SaXAudioTest, PositionTracking) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    INT32 voiceID = CreateVoice(bankID, busID, true);
    
    // Test position getting (should be 0 when not playing)
    EXPECT_EQ(GetPositionSample(voiceID), 0);
    EXPECT_FLOAT_EQ(GetPositionTime(voiceID), 0.0f);
    
    // Test total time/samples
    UINT32 totalSamples = GetTotalSample(voiceID);
    FLOAT totalTime = GetTotalTime(voiceID);
    EXPECT_GT(totalSamples, 0); // Should have some samples
    EXPECT_GT(totalTime, 0.0f); // Should have some duration
    
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

// Callback tests
TEST_F(SaXAudioTest, FinishedCallback) {
    bool callbackCalled = false;
    
    OnFinishedCallback callback = [&callbackCalled](INT32 voiceID) {
        callbackCalled = true;
    };
    
    EXPECT_NO_THROW(SetOnFinishedCallback(callback));
    
    // Note: Testing actual callback execution would require audio playback to complete
    // which is difficult in unit tests without real audio data
}

// Error handling tests
TEST_F(SaXAudioTest, InvalidVoiceOperations) {
    // Test operations on non-existent voices
    EXPECT_FALSE(Start(-1));
    EXPECT_FALSE(StartAtSample(-1, 0));
    EXPECT_FALSE(StartAtTime(-1, 0.0f));
    EXPECT_FALSE(Stop(-1));
    
    // These should not crash but may return default values
    EXPECT_NO_THROW(SetVolume(-1, 0.5f));
    EXPECT_NO_THROW(SetSpeed(-1, 1.0f));
    EXPECT_NO_THROW(SetPanning(-1, 0.0f));
    EXPECT_NO_THROW(SetLooping(-1, true));
}

// Stress tests
TEST_F(SaXAudioTest, MultipleVoicesStress) {
    auto oggBuffer = CreateDummyOggBuffer();
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    INT32 busID = CreateBus();
    
    std::vector<INT32> voiceIDs;
    
    // Create multiple voices
    for (int i = 0; i < 10; ++i) {
        INT32 voiceID = CreateVoice(bankID, busID, true);
        if (voiceID > 0) {
            voiceIDs.push_back(voiceID);
        }
    }
    
    EXPECT_GT(voiceIDs.size(), 0) << "Should create at least some voices";
    
    // Clean up
    for (INT32 voiceID : voiceIDs) {
        Stop(voiceID);
    }
    
    BankRemove(bankID);
    RemoveBus(busID);
}

// Integration test
TEST_F(SaXAudioTest, CompleteWorkflow) {
    // Complete workflow test
    auto oggBuffer = CreateDummyOggBuffer();
    
    // 1. Add to bank
    INT32 bankID = BankAddOgg(oggBuffer.data(), static_cast<UINT32>(oggBuffer.size()));
    ASSERT_GT(bankID, 0);
    
    // 2. Create bus
    INT32 busID = CreateBus();
    ASSERT_GT(busID, 0);
    
    // 3. Create voice
    INT32 voiceID = CreateVoice(bankID, busID, true);
    ASSERT_GT(voiceID, 0);
    
    // 4. Configure voice
    SetVolume(voiceID, 0.7f);
    SetSpeed(voiceID, 1.2f);
    SetPanning(voiceID, 0.3f);
    SetLooping(voiceID, true);
    SetLoopPoints(voiceID, 50, 200);
    
    // 5. Add effects
    XAUDIO2FX_REVERB_PARAMETERS reverbParams = {};
    reverbParams.WetDryMix = 30.0f;
    SetReverb(voiceID, reverbParams);
    
    // 6. Start playback
    EXPECT_TRUE(Start(voiceID));
    
    // 7. Control playback
    Pause(voiceID);
    Resume(voiceID);
    
    // 8. Clean up
    Stop(voiceID);
    BankRemove(bankID);
    RemoveBus(busID);
}

// Test fixture for testing without initialization
class SaXAudioUninitializedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Don't initialize - test behavior without initialization
    }
    
    void TearDown() override {
        // Make sure to clean up if Create was called
        Release();
    }
};

TEST_F(SaXAudioUninitializedTest, OperationsWithoutInitialization) {
    // Test that operations handle uninitialized state gracefully
    EXPECT_NO_THROW(StartEngine());
    EXPECT_NO_THROW(StopEngine());
    EXPECT_NO_THROW(PauseAll());
    EXPECT_NO_THROW(ResumeAll());
    
    // These should return invalid/default values
    EXPECT_LE(CreateBus(), 0); // Should fail
    EXPECT_FALSE(VoiceExist(1));
}