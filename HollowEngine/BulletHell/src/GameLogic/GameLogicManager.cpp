#include "GameLogicManager.h"

#include "Hollow/Managers/SceneManager.h"
#include "Hollow/Managers/ScriptingManager.h"
#include "Hollow/Managers/SystemManager.h"
#include "Hollow/Managers/ResourceManager.h"
#include "Hollow/Managers/EventManager.h"
#include "Hollow/Managers/GameObjectManager.h"
#include "Hollow/Managers/AudioManager.h"
#include "Hollow/Managers/PhysicsManager.h"
#include "Hollow/Managers/RenderManager.h"

#include "Hollow/Components/Script.h"
#include "Hollow/Components/Transform.h"
#include "Hollow/Components/Body.h"
#include "Hollow/Components/UITransform.h"
#include "Hollow/Components/UIImage.h"

#include "Hollow/Systems/PhysicsSystem.h"

#include "Components/Attack.h"
#include "Components/Pickup.h"
#include "Components/Health.h"
#include "Components/CharacterStats.h"
#include "Components/Magic.h"

#include "Hollow/Events/GameEvent.h"

#include "Events/PickupTimedEvent.h"
#include "Events/DeathEvent.h"

#include "DungeonGeneration/DungeonRoom.h"
#include "DungeonGeneration/DungeonManager.h"

#include "GameMetaData/GameEventType.h"
#include "GameMetaData/GameObjectType.h"

#include "Hollow/Utils/UniqueID.h"
#include "Hollow/Managers/InputManager.h"
#include "Hollow/Managers/FrameRateController.h"
#include "Systems/HealthSystem.h"
#include "Systems/HandSystem.h"

#define MAX_REGULAR_ROOMS 13
#define MAX_BOSS_ROOMS 4

namespace BulletHell
{
	bool GameLogicManager::hasGameStarted;
	
    GameLogicManager::GameLogicManager() 
	{
		// Preload all rooms into map
		InitializeRoomsMap();
		mPickupPrefabNames.push_back("Pickup_Damage");
		mPickupPrefabNames.push_back("Pickup_HP");
		mPickupPrefabNames.push_back("Pickup_Invincible");
		mPickupPrefabNames.push_back("Pickup_RateOfFire");
		mPickupPrefabNames.push_back("Pickup_Speed");
		HW_INFO("Initialized: GameLogicManager");
	}

    GameLogicManager& GameLogicManager::Instance()
    {
        static GameLogicManager instance;
        return instance;
    }

    void GameLogicManager::Init()
    {
		RegisterLuaBindings();
    	
    	// Init UI Window Flags
		mWindowFlags = 0;
		mWindowFlags |= ImGuiWindowFlags_NoTitleBar;
		mWindowFlags |= ImGuiWindowFlags_NoMove;
		mWindowFlags |= ImGuiWindowFlags_NoResize;
		mWindowFlags |= ImGuiWindowFlags_NoCollapse;
		mWindowFlags |= ImGuiWindowFlags_NoBackground;

		mIsFullScreen = true;
		mIsPaused = false;
		mPausingBegins = false;
		mPausingFinished = false;
    	
		InitGlobalGameObjects();

		SubscribeToEvents();
		
		CreateMainMenu();
    }
	
	void GameLogicManager::InitGlobalGameObjects()
	{
		Hollow::ScriptingManager::Instance().RunScript("InitGlobalObjects");
		sol::state& smLua = Hollow::ScriptingManager::Instance().lua;
		mpPlayerGO = smLua["player"];

    	mpPauseGO = Hollow::ResourceManager::Instance().LoadGameObjectFromFile("Resources\\Prefabs\\PauseScreen.json");
		mpPauseGO->mActive = false;
		mpSplashScreen = Hollow::ResourceManager::Instance().LoadGameObjectFromFile("Resources\\Prefabs\\SplashScreen.json");
    	
		AddGlobalGameObject(mpPlayerGO);
		AddGlobalGameObject(mpSplashScreen);
		AddGlobalGameObject(mpPauseGO);
		AddGlobalGameObject(smLua["camera"]);
		AddGlobalGameObject(smLua["UICamera"]);
		AddGlobalGameObject(smLua["globalLight"]);
	}

	void GameLogicManager::ClearNonGlobalGameObjects()
	{
		Hollow::GameObjectManager::Instance().DeleteAllGameObjectsExcept(mGlobalGameObjects);
	}

	void GameLogicManager::AddGlobalGameObject(Hollow::GameObject* pGO)
	{
		mGlobalGameObjects.push_back(pGO);
	}

	void GameLogicManager::CreateMainMenu()
	{
		hasGameStarted = false;

		Hollow::PhysicsManager::Instance().isPaused = true;

		mIsChangingFloors = false;

		// Clear non-global game objects
		ClearNonGlobalGameObjects();

		// Init globals
		Hollow::ScriptingManager::Instance().RunScript("Globals");
		
		// Call scene init for all systems
		Hollow::SystemManager::Instance().OnSceneInit();

		// Construct Room 
		ConstructMainMenuRoom();
		DungeonManager::Instance().GetCurrentRoom().UnlockRoom();

		CreateRoomLabels();

		// Set player position
		DungeonManager::Instance().mpPlayerGo = mpPlayerGO;
		Hollow::Body* pBody = mpPlayerGO->GetComponent<Hollow::Body>();
		pBody->mPosition = glm::vec3(DungeonRoom::mRoomSize + DungeonRoom::mRoomSize / 2,
			1.5,
			DungeonRoom::mRoomSize + DungeonRoom::mRoomSize / 2);

		mCreditsUIObject = Hollow::ResourceManager::Instance().LoadGameObjectFromFile("Resources\\Prefabs\\CreditsUI.json");
		mCreditsUIObject->mActive = false;

		//Hollow::SystemManager::Instance().GetSystem<Hollow::PhysicsSystem>()->UpdateTree();
		Hollow::PhysicsManager::Instance().isPaused = false;

		Hollow::AudioManager::Instance().PlaySong("Resources/Audio/Songs/MainMenu.ogg");
	}
	
	bool CheckRoomBounds(glm::vec3 playerPos, glm::vec2 coords)
    {
		if (playerPos.x > coords.y* DungeonRoom::mRoomSize
			&& playerPos.x < (coords.y + 1) * DungeonRoom::mRoomSize
			&& playerPos.z > coords.x* DungeonRoom::mRoomSize
			&& playerPos.z < (coords.x + 1) * DungeonRoom::mRoomSize)
		{
			return true;
		}

		return false;

    }
	bool fullscreen = true;
	void GameLogicManager::FireToggleFullScreenEvent()
	{
		Hollow::GameEvent ge((int)GameEventType::TOGGLE_FULLSCREEN);
		Hollow::EventManager::Instance().BroadcastToSubscribers(ge);
		mIsFullScreen ? Hollow::InputManager::Instance().ShowMouseCursor() :
			Hollow::InputManager::Instance().HideMouseCursor();
	}

	void GameLogicManager::ResetPlayerStats()
	{
		// Reset player health
		Health* pPlayerHealth = mpPlayerGO->GetComponent<Health>();
		pPlayerHealth->mHitPoints = 10;
		pPlayerHealth->mIsAlive = true;

		// Reset player stats
		// TODO: Find some way to parse this from JSON/Lua and store default values
		CharacterStats* pPlayerStats = mpPlayerGO->GetComponent<CharacterStats>();
		pPlayerStats->mMovementSpeed = 1000.0f;
		pPlayerStats->mMovementSpeedFactor = 1.0f;
		pPlayerStats->mFireRate = 1.0f;
		pPlayerStats->mDamageFactor = 1.0f;
		pPlayerStats->mDashSpeed = 1000.0f;

		// Reset collected spells
		Magic* pPlayerMagic = mpPlayerGO->GetComponent<Magic>();
		pPlayerMagic->mBasicSpells[1]->mCollected = false;
		pPlayerMagic->mBasicSpells[2]->mCollected = false;
		pPlayerMagic->mBasicSpells[4]->mCollected = false;
		pPlayerMagic->mBasicSpells[8]->mCollected = false;

		// Reset current spells
		pPlayerMagic->mLeftHandScriptPath = "Spells/Empty";
		pPlayerMagic->mRightHandScriptPath = "Spells/Empty";
		pPlayerMagic->mCombineHandScriptPath = "Spells/Empty";
		pPlayerMagic->mLeftHandSpell = nullptr;
		pPlayerMagic->mRightHandSpell = nullptr;
		pPlayerMagic->mCombinedSpell = nullptr;

		// Clear event manager so that any delayed events don't fire
		Hollow::EventManager::Instance().ClearDelayedEvents();
	}

	void GameLogicManager::CheckIfPlayerInBossRoom()
	{
		DungeonRoom& curRoom = DungeonManager::Instance().GetCurrentRoom();
		if (curRoom.mEnemies.size() > 0 &&
			curRoom.mRoomType == DungeonRoomType::BOSS)
		{
			// Play boss theme
			Hollow::AudioManager::Instance().PlaySong("Resources/Audio/Songs/BossTheme.ogg");
		}
	}

	void GameLogicManager::TogglePause()
	{
		mPausingBegins = true;
	}

	void GameLogicManager::CheckForPause()
	{
		if (mPausingBegins)
		{
			mIsPaused = !mIsPaused;
			Hollow::AudioManager::Instance().Mute(mIsPaused);
			mpPauseGO->mActive = mIsPaused;

			mPausingBegins = false;
			mPausingFinished = true;
		}
		else if(mPausingFinished)
		{
			Hollow::GameEvent ge((int)BulletHell::GameEventType::TOGGLE_PAUSE);
			Hollow::EventManager::Instance().BroadcastToSubscribers(ge);
			mPausingFinished = false;
		}
	}
	
	void GameLogicManager::Update()
	{
		CheckCheatCodes();
		CheckKillPlane();
		CheckIfPlayerInBossRoom();

		if (Hollow::InputManager::Instance().IsKeyTriggered("F11"))
		{
			FireToggleFullScreenEvent();
			mIsFullScreen = !mIsFullScreen;
			fullscreen = mIsFullScreen;
		}

		if (Hollow::InputManager::Instance().IsKeyTriggered("Escape") ||
			Hollow::InputManager::Instance().IsControllerButtonTriggered(SDL_CONTROLLER_BUTTON_START))
		{
			TogglePause();
		}
		
		CheckForPause();
		
		UpdateSplashScreen();

		// Reset ImGUI gamepad controls
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = 0;
		
		if (mIsPaused)
		{
			CreatePauseUI();
		}
		
		if(hasGameStarted)
    	{
			return;
    	}
		glm::vec3 playerPos = mpPlayerGO->GetComponent<Hollow::Transform>()->mPosition;
    	
    	if(CheckRoomBounds(playerPos, glm::vec2(0,1)))
    	{
    		StartNewGame();
    	}
		else if (CheckRoomBounds(playerPos, glm::vec2(1, 0)))
		{
			CreateOptionsUI();
		}
		else if (CheckRoomBounds(playerPos, glm::vec2(2, 1)))
		{
			CreateExitConfirmationUI();
		}
		else if (CheckRoomBounds(playerPos, glm::vec2(1, 2)))
		{
			if(!mCreditsUIObject->mActive)
				CreateCreditsUI();

			Hollow::ScriptingManager::Instance().RunScript("slideshow", mCreditsUIObject);
		}
		else
		{
			mCreditsUIObject->mActive = false;
		}
	}

	void GameLogicManager::StartNewGame()
    {
		hasGameStarted = true;
		Hollow::PhysicsManager::Instance().isPaused = true;
    	
		// Configure game settings
		Hollow::ScriptingManager::Instance().RunScript("GameConfig");

		// Clear non-global game objects
		ClearNonGlobalGameObjects();

		// Call scene init for all systems
		Hollow::SystemManager::Instance().OnSceneInit();

		// Reset dungeon values and init with values from GameConfig.lua
		BulletHell::DungeonManager::Instance().ConfigureDungeon();
		BulletHell::DungeonManager::Instance().Regenerate();
		BulletHell::DungeonManager::Instance().Init();

		// Setup bosses/spell order
		// TODO: Find a better way to use the seed in the dungeon generator
		mSpellOrder = { 1, 2, 3, 4 };
		std::shuffle(mSpellOrder.begin(), mSpellOrder.end(), std::default_random_engine(DungeonManager::Instance().GetSeed()));
		HW_INFO("Spell Order: {0}, {1}, {2}, {3}", mSpellOrder[0], mSpellOrder[1], mSpellOrder[2], mSpellOrder[3]);

		Hollow::ScriptingManager::Instance().RunScript("SetupLevel");
		
		mRandomCount = 3; // first drop after 3 enemies... then randomize
		mCountDeadEnemies = 0;

    	
		Hollow::SystemManager::Instance().GetSystem<Hollow::PhysicsSystem>()->UpdateTree();
		Hollow::PhysicsManager::Instance().isPaused = false;

		// Start playing ambient dungeon theme
		Hollow::AudioManager::Instance().PlaySong("Resources/Audio/Songs/AmbientDungeonTheme.ogg");
		// Play teleport sounds
		Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/Teleport.wav");
    }

	void GameLogicManager::RegisterLuaBindings()
	{
		// LUA Bindings for Dungeon Classes
		// Send attack component to lua
		auto& lua = Hollow::ScriptingManager::Instance().lua;

		lua.new_usertype<DungeonFloor>("DungeonFloor",
			sol::constructors<DungeonFloor()>(),
			"GetRoom", &DungeonFloor::GetRoom,
			"GetRoomFromIndex", &DungeonFloor::GetRoomFromIndex,
			"GetFloorNum", &DungeonFloor::GetFloorNumber,
			"GetRegularRoom", &DungeonFloor::GetRegularRoom,
			"GetRoomCount", &DungeonFloor::GetRoomCount,
			"GetEntrance", &DungeonFloor::GetEntrance,
			"GetEntranceIndex", &DungeonFloor::GetEntranceIndex,
			"GetBossIndex", &DungeonFloor::GetBossIndex
			);

		lua.new_usertype<DungeonRoom>("DungeonRoom",
			sol::constructors<DungeonRoom()>(),
			"GetFloorNum", &DungeonRoom::GetFloorNum,
			"GetDoorBits", &DungeonRoom::GetDoorBits,
			"GetCoords", &DungeonRoom::GetCoords,
			"IsCleared", &DungeonRoom::IsCleared,
			"UnlockRoom", &DungeonRoom::UnlockRoom,
			"LockDownRoom", &DungeonRoom::LockDownRoom,
			"getEnemyCount", &DungeonRoom::GetEnemyCount,
			"GetID", &DungeonRoom::GetID,
			"Enemies", &DungeonRoom::mEnemies,
			"obstacles", &DungeonRoom::mObstacles,
			"roomType", &DungeonRoom::mRoomType
			);

		lua.set_function("HandleBulletDamage", &HealthSystem::HandleBulletDamage, std::ref(*Hollow::SystemManager::Instance().GetSystem<HealthSystem>()));
		lua.set_function("GetDungeonFloor", &DungeonManager::GetFloor, std::ref(DungeonManager::Instance()));
		lua.set_function("PopulateRoom", &GameLogicManager::PopulateRoom, std::ref(GameLogicManager::Instance()));
		lua.set_function("CreatePickUpInRoom", &GameLogicManager::CreatePickUpInRoom, std::ref(GameLogicManager::Instance()));
		lua.set_function("RegenerateDungeon", &DungeonManager::Regenerate, std::ref(DungeonManager::Instance()));
		lua.set_function("OnRoomEntered", &DungeonManager::OnCurrentRoomUpdated, std::ref(DungeonManager::Instance()));
		lua.set_function("DCastRay", &DungeonManager::CastRay, std::ref(DungeonManager::Instance()));
		lua.set_function("MoveToNextFloor", &GameLogicManager::MoveToNextFloor, std::ref(GameLogicManager::Instance()));
		lua.set_function("GetSpellOrder", &GameLogicManager::GetSpellOrder, std::ref(GameLogicManager::Instance()));
	}

	void GameLogicManager::MoveToNextFloor()
    {
        auto& lua = Hollow::ScriptingManager::Instance().lua;
        int currentFloor = lua["currentFloor"].get<int>();
        DungeonManager::Instance().GetFloor(currentFloor).ResetFloor();
        currentFloor++;

		HW_TRACE("Current Floor: {0}", currentFloor);

		// Go back to main menu if end of game is reached
		if (currentFloor == lua["numFloors"].get<int>())
		{
			DungeonManager::Instance().Regenerate();
            // play sound "Congratulaition! You win!"or somthing like that
			Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/Win.wav");
			CreateMainMenu();
            Hollow::ResourceManager::Instance().LoadPrefabAtPosition("YouWin", glm::vec3(30.0f, 0.6f, 30.0f));
		}
		else
		{
			lua["currentFloor"] = currentFloor;
			ClearNonGlobalGameObjects();
			DungeonManager::Instance().Construct(currentFloor);

			Hollow::ScriptingManager::Instance().RunScript("SetupLevel");
        
			BulletHell::DungeonManager::Instance().mpPlayerGo = mpPlayerGO;
			Hollow::SystemManager::Instance().OnSceneInit();
			// Start playing ambient dungeon theme
			Hollow::AudioManager::Instance().PlaySong("Resources/Audio/Songs/AmbientDungeonTheme.ogg");
			// Play teleport sounds
			Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/Teleport.wav");

		}

		// Reset player speed to normal
		// Instead of messing with stats, just set velocity to zero ya goob
		mIsChangingFloors = false;
    }

	void GameLogicManager::CreateRoomLabels()
	{
		// Place Text Objects
		Hollow::ResourceManager::Instance().LoadPrefabAtPosition("Options", glm::vec3(21.5f, 0.5f, 30.0f));
		Hollow::ResourceManager::Instance().LoadPrefabAtPosition("Credits", glm::vec3(38.5f, 0.5f, 30.0f));
		Hollow::ResourceManager::Instance().LoadPrefabAtPosition("NewGame", glm::vec3(30.0f, 0.5f, 21.5f));
		Hollow::ResourceManager::Instance().LoadPrefabAtPosition("Exit", glm::vec3(30.0f, 0.5f, 38.5f));    	
	}

	void GameLogicManager::CreateOptionsUI()
	{
    	
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
		ImGui::Begin("Audio", nullptr, mWindowFlags);
		ImGui::SliderFloat("Master Volume", &Hollow::AudioManager::Instance().mMasterVolume, 0.0f, 1.0f);
		ImGui::Checkbox("Mute Songs", &Hollow::AudioManager::Instance().mMute[Hollow::SOUND_BACKGROUND]);
		ImGui::SliderFloat("Song Volume", &Hollow::AudioManager::Instance().mVolume[Hollow::SOUND_BACKGROUND], 0.0f, 1.0f);
		ImGui::Checkbox("Mute SFX", &Hollow::AudioManager::Instance().mMute[Hollow::SOUND_EFFECT]);
		ImGui::SliderFloat("SFX Volume", &Hollow::AudioManager::Instance().mVolume[Hollow::SOUND_EFFECT], 0.0f, 1.0f);
		ImGui::Checkbox("FullScreen", &fullscreen);
		//ImGui::Button("Back", ImVec2(100, 50));
		ImGui::End();

    	if(fullscreen != mIsFullScreen)
    	{
			FireToggleFullScreenEvent();
			mIsFullScreen = fullscreen;
    	}
	}

	void GameLogicManager::CreateCreditsUI()
	{
		if (!mCreditsUIObject->mActive)
		{
			std::string creditsPath = "Resources/Textures/UI/Credits/";

			std::vector<std::string> paths;
			for (int i = 1; i <= 7; ++i)
			{
				paths.push_back(creditsPath + std::to_string(i) + ".png");
			}

			mCreditsUIObject->GetComponent<Hollow::UIImage>()->SetTexture("Resources/Textures/UI/Credits/1.png");
			
			Hollow::ScriptingManager::Instance().lua["CreditsPaths"] = paths;
			Hollow::ScriptingManager::Instance().lua["slideNum"] = 2;

			mCreditsUIObject->mActive = true;
		}
		
	}

	void GameLogicManager::CreatePauseUI()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(200, 150));
		ImGui::Begin("Pause", nullptr, mWindowFlags);
		if (ImGui::Button("Resume", ImVec2(100, 50)))
		{
			TogglePause();
		}
		if (ImGui::Button("Main Menu", ImVec2(100, 50)))
		{
			TogglePause();
			// Reset player stats
			ResetPlayerStats();
			HandSystem::ResetHands();
			DungeonManager::Instance().Regenerate();
			CreateMainMenu();
		}
		if(ImGui::IsItemHovered())
		{
			ImGui::Text("Progress will not be saved");
		}
		//ImGui::Button("No", ImVec2(100, 50));
		ImGui::End();
	}
	

	void GameLogicManager::CreateExitConfirmationUI()
	{
		glm::ivec2 screenSize = Hollow::RenderManager::Instance().GetWindowSize();
    	
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
		ImGui::Begin("Exit", nullptr, mWindowFlags);
		ImGui::Text("Are You Sure?");
		ImGui::Text("Progress will not be saved");
    	if(ImGui::Button("Totally", ImVec2(100, 50)))
    	{
			Hollow::GameEvent ge((int)GameEventType::ON_EXIT_GAME);
			Hollow::EventManager::Instance().BroadcastToSubscribers(ge);
    	}
		//ImGui::Button("No", ImVec2(100, 50));
		ImGui::End();
	}

	void GameLogicManager::SubscribeToEvents()
    {
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ROOM_LOCKDOWN_DELAYED, EVENT_CALLBACK(GameLogicManager::OnRoomLockDownDelayed));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_PICKUP_COLLECT, EVENT_CALLBACK(GameLogicManager::OnPickupCollected));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_PICKUP_EFFECT_END, EVENT_CALLBACK(GameLogicManager::OnPickupEffectEnd));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_BULLET_HIT_SHIELD, EVENT_CALLBACK(GameLogicManager::OnBulletHitShield));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::DEATH, EVENT_CALLBACK(GameLogicManager::OnDeath));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::PLAYER_DEATH, EVENT_CALLBACK(GameLogicManager::OnPlayerDeath));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_PLAYER_HIT_PORTAL, EVENT_CALLBACK(GameLogicManager::OnPlayerHitPortal));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_SPELL_COLLECT, EVENT_CALLBACK(GameLogicManager::OnSpellCollect));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::FLOOR_CLEARED_DELAYED, EVENT_CALLBACK(GameLogicManager::OnFloorCleared));
    }

	void GameLogicManager::OnPickupCollected(Hollow::GameEvent& event)
	{
		// Find out the pickup gameobject 
		Hollow::GameObject* pPickupObject = event.mpObject1->mType == (int)GameObjectType::PICKUP ? event.mpObject1 : event.mpObject2;
		Hollow::GameObject* pPlayer = event.mpObject1->mType == (int)GameObjectType::PLAYER ? event.mpObject1 : event.mpObject2;

		Pickup* pPickup = pPickupObject->GetComponent<Pickup>();
		AddBuffs(pPlayer, pPickup);

		// Play a nice sound effect
		Hollow::ScriptingManager::Instance().RunScript("Destroy/PlaySoundEffectPickup", pPickupObject);

		if (pPickup->mEffectTime > 0.0f)
		{
			float effectTime = pPickup->mEffectTime;

			// change pickup so that its effects are reversed after the given time
			pPickup->mEffectTime = 0.0f;
			pPickup->mBuffValue = -pPickup->mBuffValue;
			
			// create a pickup timed event
			PickupTimedEvent* pEvent = new PickupTimedEvent(pPickup);
			pEvent->mpObject1 = pPlayer;

			Hollow::EventManager::Instance().AddDelayedEvent(pEvent, effectTime);
			
		}
		Hollow::GameObjectManager::Instance().DeleteGameObject(pPickupObject);
		
		//Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/OnPickup.wav");
	}

	void GameLogicManager::OnPickupEffectEnd(Hollow::GameEvent& event)
	{
		PickupTimedEvent* pte = dynamic_cast<PickupTimedEvent*>(&event);

		// Add the buffs to the object
		AddBuffs(pte->mpObject1, &pte->mPickup);
	}

	void GameLogicManager::ConstructMainMenuRoom()
    {
		HW_TRACE("Creating MainMenu");
    	// Setup variables for MainMenuRoom construction
		DungeonManager::Instance().ConfigureDungeon();
    	
		DungeonFloor mainMenuFloor(3, 3, 0);
		mainMenuFloor.GetRoom(0, 1).Set(DungeonRoomType::REGULAR, Hollow::GenerateUniqueID<DungeonRoom>(), DungeonRoom::DoorDirrection::DOWN, 0, 0, 1);
		mainMenuFloor.GetRoom(2, 1).Set(DungeonRoomType::REGULAR, Hollow::GenerateUniqueID<DungeonRoom>(), DungeonRoom::DoorDirrection::UP, 0, 2, 1);
		mainMenuFloor.GetRoom(1, 0).Set(DungeonRoomType::OPTIONS, Hollow::GenerateUniqueID<DungeonRoom>(), DungeonRoom::DoorDirrection::RIGHT, 0, 1, 0);
		mainMenuFloor.GetRoom(1, 2).Set(DungeonRoomType::CREDITS, Hollow::GenerateUniqueID<DungeonRoom>(), DungeonRoom::DoorDirrection::LEFT, 0, 1, 2);
		mainMenuFloor.GetRoom(1, 1).Set(DungeonRoomType::MAIN_MENU, Hollow::GenerateUniqueID<DungeonRoom>(), 15, 0, 1, 1);
		
    	mainMenuFloor.ConstructFloor();

    	DungeonManager::Instance().GetFloors().push_back(mainMenuFloor);
    }

    Hollow::GameObject* GameLogicManager::GenerateObjectAtPosition(std::string prefabName, glm::ivec2 roomCoords, glm::vec2 posOffset)
    {
		Hollow::GameObject* pGo = Hollow::ResourceManager::Instance().LoadPrefabAtPosition(prefabName,
			glm::vec3(roomCoords.y * DungeonRoom::mRoomSize + posOffset.x,
				1.5f,
				roomCoords.x * DungeonRoom::mRoomSize + posOffset.y));

        // TODO adjust enemy level according to dungeon floor

        // Disable any scripts
        Hollow::Script* pS = pGo->GetComponent<Hollow::Script>();
		if (pS != nullptr)
		{
			pS->mIsActive = false;
		}
        Attack* pA = pGo->GetComponent<Attack>();
		if (pA != nullptr)
		{
			pA->mIsActive = false;
		}

        return pGo;
    }

    void GameLogicManager::OnRoomLockDownDelayed(Hollow::GameEvent& event)
    {
		auto room = DungeonManager::Instance().GetCurrentRoom();
		for (auto enemy : room.mEnemies)
		{
			// Enable any scripts
			Hollow::Script* pS = enemy->GetComponent<Hollow::Script>();
			if (pS != nullptr)
			{
				pS->mIsActive = true;
			}
			Attack* pA = enemy->GetComponent<Attack>();
			if (pA != nullptr)
			{
				pA->mIsActive = true;
			}
		}
    }

	void GameLogicManager::OnSpellCollect(Hollow::GameEvent& event)
	{
		DungeonRoom& currentRoom = DungeonManager::Instance().GetCurrentRoom();

		// Mainly used for unlocking starting room
		currentRoom.UnlockRoom();

		// Play a nice sound effect
		Hollow::GameObject* pSpellObject = event.mpObject1->mType == (int)GameObjectType::SPELL ? event.mpObject1 : event.mpObject2;
		Hollow::ScriptingManager::Instance().RunScript("Destroy/SpellCollected", pSpellObject);

		// Spawn portal to next floor if not in starting room
		if (currentRoom.mRoomType == DungeonRoomType::BOSS)
		{
			glm::vec3 portalPosition = currentRoom.GetCenter();
			portalPosition.y = 1.5f;
			portalPosition.z += 3.0f;
			// TODO: Check if too close to the player
			//glm::vec3 playerPosition = mpPlayerGO->GetComponent<Hollow::Transform>()->mPosition;
			Hollow::ResourceManager::Instance().LoadPrefabAtPosition("Portal", portalPosition);
		}
	}

	void GameLogicManager::OnFloorCleared(Hollow::GameEvent& event)
	{
		MoveToNextFloor();
	}

	void GameLogicManager::CheckCheatCodes()
	{
		Hollow::ScriptingManager::Instance().RunScript("CheatCodes");
	}

	void GameLogicManager::UpdateSplashScreen()
	{
		float dt = Hollow::FrameRateController::Instance().GetFrameTime();

		splashTime += dt;

		if (splashTime > 3.0f)
		{
			Hollow::UIImage* image = mpSplashScreen->GetComponent<Hollow::UIImage>();
			image->mAlpha -= 0.01f;

			if(image->mAlpha < -1.0f)
			{
				mpSplashScreen->mActive = false;
			}
		}
	}

	void GameLogicManager::CheckKillPlane()
	{
		// Check if any objects are lower than kill plane and delete them if so
		float minYVal = -1.5f;
		for (auto& gameObject : Hollow::GameObjectManager::Instance().GetGameObjects())
		{
			Hollow::Transform* pTr = gameObject->GetComponent<Hollow::Transform>();
			if (pTr && pTr->mPosition.y < minYVal)
			{
				// Send event to destroy object
				DeathEvent death(gameObject->mType);
				death.mpObject1 = gameObject;
				Hollow::EventManager::Instance().BroadcastToSubscribers(death);
				Hollow::GameObjectManager::Instance().DeleteGameObject(gameObject);
			}
		}
	}

	void GameLogicManager::InitializeRoomsMap()
	{
		// Loads all room files into map
		// for all room files
		LoadRoomJsons("Room", MAX_REGULAR_ROOMS);

		// Do the same for boss rooms
		LoadRoomJsons("Boss/Room", MAX_BOSS_ROOMS);
	}

	void GameLogicManager::AddBuffs(Hollow::GameObject* pPlayer, Pickup* pPickup)
	{
		CharacterStats* pStats = pPlayer->GetComponent<CharacterStats>();

		switch (pPickup->mPickupType)
		{
		case PickupType::HP:
		{
			Health* pHp = pPlayer->GetComponent<Health>();
			pHp->mHitPoints += pPickup->mBuffValue;
			break;
		}
		case PickupType::DASH:
		{
			pStats->mDashSpeed += pPickup->mBuffValue;
			break;
		}
		case PickupType::DAMAGE_FACTOR:
		{
			pStats->mDamageFactor += pPickup->mBuffValue;
			break;
		}
		case PickupType::SPEED_FACTOR:
		{
			pStats->mMovementSpeedFactor += pPickup->mBuffValue;
			HW_TRACE("Giving the player {0} speed for {1}", pPickup->mBuffValue, pPickup->mEffectTime);
			break;
		}
		case PickupType::SPEED:
		{
			pStats->mMovementSpeed += pPickup->mBuffValue;
			break;
		}
		case PickupType::RATE_OF_FIRE:
		{
			pStats->mFireRate += pPickup->mBuffValue;
			break;
		}
		case PickupType::INVINCIBILITY:
		{
			Health* pHp = pPlayer->GetComponent<Health>();
			pHp->mInvincibleTime += pPickup->mBuffValue;
			break;
		}
		}
	}

	void GameLogicManager::LoadRoomJsons(std::string roomPrefix, int count)
	{
		std::string path = "Resources/Json data/Rooms/";
		for (int i = 1; i <= count; ++i)
		{
			std::string name = roomPrefix + std::to_string(i);
			std::ifstream file(path + name + ".json");
			std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
			mCachedRoomsMap[name] = contents;
		}
	}

	void GameLogicManager::OnBulletHitShield(Hollow::GameEvent& event)
	{
		// Destroy the enemy bullet and spawn a new player bullet that tracks the nearest enemy
		Hollow::GameObject* pBullet = event.mpObject1->mType == (int)GameObjectType::BULLET ? event.mpObject1 : event.mpObject2;
		Hollow::ScriptingManager::Instance().RunScript("CreateWater", pBullet);
		Hollow::GameObjectManager::Instance().DeleteGameObject(pBullet);
	}

	void GameLogicManager::PopulateRoom(DungeonRoom& room)
	{
		int roomNum = 0; 
		std::string roomName;
		if(room.mRoomType == DungeonRoomType::BOSS)
		{
			// Get boss room based on spell order
			auto& lua = Hollow::ScriptingManager::Instance().lua;
			int currentFloor = lua["currentFloor"].get<int>();
			roomNum = mSpellOrder[currentFloor];
			roomName = "Boss/Room" + std::to_string(roomNum);
			HW_WARN("Spawning boss: {0}", roomName);
		}
		else
		{
			roomNum = Random::RangeSeeded(1, MAX_REGULAR_ROOMS);
            roomName = "Room" + std::to_string(roomNum);
		}
		
		rapidjson::Document root;
		root.Parse(mCachedRoomsMap[roomName].c_str());
		if (!root.IsObject()) { HW_CORE_ERROR("Error reading Room file {0}", roomName); }
		rapidjson::Value::Object data = root.GetObject();

		if(data.HasMember("Enemies"))
		{
			CreateObjectsInRoom(room, data, "Enemies", "EnemyType", true);
		}

		if(data.HasMember("Obstacles"))
		{
			CreateObjectsInRoom(room, data, "Obstacles", "ObstacleType", false);
		}
		
	}

	void GameLogicManager::CreateObjectsInRoom(DungeonRoom& room, const rapidjson::Value::Object& data, const std::string& objects, const std::string& type, bool addToEnemylist)
	{
		glm::ivec2 coords = room.GetCoords();
		// Parse all objects in file
		std::string prefabName = "";
		glm::vec2 position(0.0f, 0.0f);
		if (data[objects.c_str()].IsArray())
		{
			auto objectArray = data[objects.c_str()].GetArray();
			for (int objectIndex = 0; objectIndex < objectArray.Size(); ++objectIndex)
			{
				auto object = objectArray[objectIndex].GetObject();
				// Get object type, this is the json prefab file that will be loaded
				if (object[type.c_str()].IsString())
				{
					prefabName = object[type.c_str()].GetString();
				}
				// Get object position
				if (object["Position"].IsArray())
				{
					auto pos = object["Position"].GetArray();
					position = glm::vec2(pos[0].GetFloat(), pos[1].GetFloat());
				}

				// Load enemy prefab at position relative to room
				Hollow::GameObject* pGo = GenerateObjectAtPosition(prefabName, coords, position);
				if (addToEnemylist)
				{
					room.mEnemies.push_back(pGo);
				}
				else
				{
					room.mObstacles.push_back(pGo);
				}
			}
		}
	}

	void GameLogicManager::DropRandomPickup(Hollow::GameObject* pGO)
	{
		// Assuming every death event is sent by an ENEMY
		mCountDeadEnemies++;

		if (mCountDeadEnemies > mRandomCount)
		{
			mCountDeadEnemies = 0;
			mRandomCount = Random::RangeSeeded(5, 9);

			int randomIndex = Random::RangeSeeded(0, mPickupPrefabNames.size()-1);
				
			Hollow::Transform* pTr = pGO->GetComponent<Hollow::Transform>();
			Hollow::ResourceManager::Instance().LoadPrefabAtPosition(mPickupPrefabNames[randomIndex], pTr->mPosition);
		}
	}

	void GameLogicManager::OnDeath(Hollow::GameEvent& event)
	{
		DeathEvent& pDeathEvent = dynamic_cast<DeathEvent&>(event);
		if (pDeathEvent.mType == (int)GameObjectType::ENEMY)
		{
			// Bosses drop spells through their destruction scripts
			if (pDeathEvent.mpObject1->mTag != "Boss")
			{
				DropRandomPickup(pDeathEvent.mpObject1);
			}
			else
			{
				// Change music back
				Hollow::AudioManager::Instance().PlaySong("Resources/Audio/Songs/AmbientDungeonTheme.ogg");
			}
			// Play a nice sound effect
			Hollow::ScriptingManager::Instance().RunScript("Destroy/PlaySoundEffectEnemy", pDeathEvent.mpObject1);
		}
	}

	void GameLogicManager::OnPlayerDeath(Hollow::GameEvent& event)
	{
		HW_WARN("Player Death");

		// Kick back to main menu
		DungeonManager::Instance().Regenerate();
		CreateMainMenu();    
        Hollow::ResourceManager::Instance().LoadPrefabAtPosition("YouLose", glm::vec3(30.0f, 0.6f, 30.0f));

		// Reset player stats
		ResetPlayerStats();

		// Reset intial values in any systems/components e.g. Spell Collected flag
	}

	void GameLogicManager::OnPlayerHitPortal(Hollow::GameEvent& event)
	{
		if (!mIsChangingFloors)
		{
			Hollow::GameEvent* fce = new Hollow::GameEvent((int)GameEventType::FLOOR_CLEARED_DELAYED);
			// Send delayed event
			Hollow::EventManager::Instance().AddDelayedEvent(fce, 2.0f);

			mIsChangingFloors = true;
		}
		// Stop player from moving
		Hollow::Body* pBody = mpPlayerGO->GetComponent<Hollow::Body>();
		pBody->mVelocity = glm::vec3(0.0f);
	}

    void GameLogicManager::CreatePickUpInRoom(DungeonRoom& room)
    {
        glm::ivec2 coords = room.GetCoords();
		int randomIndex = Random::RangeSeeded(0, mPickupPrefabNames.size()-1);

        Hollow::GameObject* pGo = Hollow::ResourceManager::Instance().LoadPrefabAtPosition(
				mPickupPrefabNames[randomIndex],
             glm::vec3(coords.y * DungeonRoom::mRoomSize + DungeonRoom::mRoomSize / 2,
                1.5,
                coords.x * DungeonRoom::mRoomSize + DungeonRoom::mRoomSize / 2));

    	// make the effect time zero ie make it permanent
    	Pickup* pP = pGo->GetComponent<Pickup>();
		pP->mEffectTime = 0.0f;
    	// make the effect stronger
		pP->mBuffValue = Random::RangeSeeded(1.2f, 2.0f) * pP->mBuffValue;
    }
}
