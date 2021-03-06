#include <hollowpch.h>

#include "AudioManager.h"
#include "ResourceManager.h"
#include "ImGuiManager.h"

namespace Hollow {
	void AudioManager::Init(rapidjson::Value::Object& data)
	{
		FMOD_RESULT result = FMOD_RESULT::FMOD_OK;

		// Create a FMOD Audio system
		result = FMOD::System_Create(&mpSystem);
		if (result != FMOD_RESULT::FMOD_OK)
		{
			HW_CORE_ERROR("Unable to create FMOD audio system");
		}

		// Check version
		unsigned int version = 0;
		result = mpSystem->getVersion(&version);
		if (result != FMOD_RESULT::FMOD_OK)
		{
			HW_CORE_ERROR("FMOD Lib version doesn't match header version");
		}

		// Initialize system with 32 channels
		void* extraDriverData = 0;
		result = mpSystem->init(32, FMOD_INIT_NORMAL, extraDriverData);
		if (result != FMOD_RESULT::FMOD_OK)
		{
			HW_CORE_ERROR("Unable to initialize FMOD audio system");
		}

		// Create channel groups
		mpSystem->getMasterChannelGroup(&mpMasterChannel);
		for (unsigned int channelIndex = 0; channelIndex < SOUND_NUM; ++channelIndex)
		{
			mpSystem->createChannelGroup(0, &mpChannelGroups[channelIndex]);
			mpMasterChannel->addGroup(mpChannelGroups[channelIndex]);
		}

		// Set sound modes for each category
		mModes[SOUND_EFFECT] = FMOD_DEFAULT | FMOD_LOOP_OFF;
		mModes[SOUND_BACKGROUND] = FMOD_DEFAULT | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL;
		mModes[SOUND_UI] = FMOD_DEFAULT | FMOD_LOOP_OFF;

		// Set starting songs and audio levels based on settings file
		mMasterVolume = data["Volumes"].GetObject()["Master"].GetFloat();
		mVolume[SOUND_BACKGROUND] = data["Volumes"].GetObject()["Music"].GetFloat();
		mVolume[SOUND_EFFECT] = data["Volumes"].GetObject()["SFX"].GetFloat();
		PlaySong(data["Background"].GetString());

		// Add to ImGui debug display
		ImGuiManager::Instance().AddDisplayFunction("Audio", std::bind(&AudioManager::DebugDisplay, &AudioManager::Instance()));
	}

	void AudioManager::CleanUp()
	{
		mpSystem->release();
		delete mpSystem;
	}

	void AudioManager::Update()
	{
		// Adjust channel volume
		SetVolume();
		mpMasterChannel->setVolume(mMasterVolume);

		MuteChannel(SOUND_BACKGROUND);
		MuteChannel(SOUND_EFFECT);
		MuteChannel(SOUND_UI);
		
		// Update FMOD audio system
		mpSystem->update();
	}

	void AudioManager::PlaySong(const std::string& songName)
	{
		FMOD::Sound* song = ResourceManager::Instance().LoadSound(songName, mModes[SOUND_BACKGROUND]);
		if (song == nullptr)
		{
			HW_CORE_WARN("Unable to load song {0}", songName);
		}
		if (songName == mCurrentSongPath)
		{
			return;
		}
		mpCurrentSong->setVolume(0.0f);
		// Set current song
		mCurrentSongPath = songName;
		mpSystem->playSound(song, 0, true, &mpCurrentSong);
		mpCurrentSong->setChannelGroup(mpChannelGroups[SOUND_BACKGROUND]);
		mpCurrentSong->setPaused(false);
	}

	void AudioManager::PlayEffect(const std::string& effectName)
	{
		FMOD::Sound* pSound = ResourceManager::Instance().LoadSound(effectName, mModes[SOUND_EFFECT]);
		FMOD::Channel* pChannel = nullptr;
		FMOD_RESULT result = mpSystem->playSound(pSound, 0, false, &pChannel);

		// Set the audio channel
		pChannel->setChannelGroup(mpChannelGroups[SOUND_EFFECT]);
		pChannel->setPaused(false);
	}

	void AudioManager::Mute(bool muteFlag)
	{
		mMute[SOUND_BACKGROUND] = muteFlag;
		mMute[SOUND_EFFECT] = muteFlag;
		mMute[SOUND_UI] = muteFlag;
		MuteChannel(SOUND_BACKGROUND);
		MuteChannel(SOUND_EFFECT);
		MuteChannel(SOUND_UI);
	}

	void AudioManager::MuteChannel(SOUND_TYPE channel)
	{
		mpChannelGroups[channel]->setMute(mMute[channel]);
	}

	void AudioManager::SetVolume()
	{
		SetChannelVolume(SOUND_BACKGROUND);
		SetChannelVolume(SOUND_EFFECT);
		SetChannelVolume(SOUND_UI);
	}

	void AudioManager::SetChannelVolume(SOUND_TYPE channel)
	{
		mpChannelGroups[channel]->setVolume(mVolume[channel]);
	}

	void AudioManager::DebugDisplay()
	{
		ImGui::SliderFloat("Master Volume", &mMasterVolume, 0.0f, 1.0f);
		mpMasterChannel->setVolume(mMasterVolume);
		DebugDisplaySongs();
		DebugDisplaySFX();
	}

	void AudioManager::DebugDisplaySongs()
	{
		ImGui::Checkbox("Mute Songs", &mMute[SOUND_BACKGROUND]);
		ImGui::SliderFloat("Song Volume", &mVolume[SOUND_BACKGROUND], 0.0f, 1.0f);
	}

	void AudioManager::DebugDisplaySFX()
	{
		ImGui::Checkbox("Mute SFX", &mMute[SOUND_EFFECT]);
		ImGui::SliderFloat("SFX Volume", &mVolume[SOUND_EFFECT], 0.0f, 1.0f);
		if (ImGui::Button("Test SFX"))
		{
			PlayEffect("Resources/Audio/SFX/1UP.wav");
		}
	}

}  // namespace Hollow