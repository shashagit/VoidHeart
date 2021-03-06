#include <Hollow.h>
#include "HealthSystem.h"

#include "Hollow/Managers/EventManager.h"
#include "Hollow/Managers/GameObjectManager.h"
#include "Hollow/Managers/InputManager.h"
#include "Hollow/Managers/FrameRateController.h"
#include "Hollow/Managers/AudioManager.h"
#include "Hollow/Managers/ResourceManager.h"
#include "Hollow/Managers/RenderManager.h"
#include "Hollow/Managers/LocalizationManager.h"
#include "Hollow/Managers/PhysicsManager.h"
#include "Hollow/Managers/ScriptingManager.h"

#include "Hollow/Components/UIText.h"
#include "Hollow/Components/Transform.h"
#include "Hollow/Components/UIImage.h"
#include "Hollow/Components/UITransform.h"

#include "GameMetaData/GameEventType.h"
#include "GameMetaData/GameObjectType.h"

#include "Components/CharacterStats.h"

#include "Events/DeathEvent.h"
#include "Events/PlayerDeathEvent.h"

namespace BulletHell
{
	HealthSystem HealthSystem::instance;
	
	void HealthSystem::Init()
	{
		// Set event callback functions
		SubscribeToEvents();

		std::ifstream file("Resources/GameData/BulletDamageValues.data");
		std::string line;

		if (file.is_open())
		{
			while (getline(file, line))
			{
				std::istringstream iss(line);
				std::vector<std::string> results(std::istream_iterator<std::string>{iss},
					std::istream_iterator<std::string>());

				mMapBulletDamage[results[0]] = stoi(results[1]);
			}
			file.close();
		}
		
	}

	void HealthSystem::OnSceneInit()
	{
		// Clear old UIIcons
		mPlayerHPUIIcons.clear();
		// Readd new UIIcons
	}

	void HealthSystem::Update()
	{
		for (unsigned int i = 0; i < mGameObjects.size(); ++i)
		{
			Health* pHealth = mGameObjects[i]->GetComponent<Health>();		
			
			// Update player HP bar
			//Hollow::UIText* pHPText = mGameObjects[i]->GetComponent<Hollow::UIText>();
			//std::string ss = Hollow::LocalizationManager::Instance().mCurrentLanguageMap[pHPText->mTag];
			//pHPText->mText = ss + std::to_string(pHealth->mHitPoints);
			
			// Update invincibility time
			if (pHealth->mInvincible)
			{
				pHealth->mCurrentInvincibleTime += Hollow::FrameRateController::Instance().GetFrameTime();
				
				// Check if should still be invincible
				if (pHealth->mCurrentInvincibleTime > pHealth->mInvincibleTime)
				{
					pHealth->mInvincible = false;
					pHealth->mCurrentInvincibleTime = 0.0;
				}
			}

			if (pHealth->mHitPoints < 0)
			{
				pHealth->mIsAlive = false;
                
				// Check type of object destroyed
				if (mGameObjects[i]->mType == (int)GameObjectType::PLAYER)
				{
                    // time for deathAnimation
                    pHealth->mCurrentDeathTime += Hollow::FrameRateController::Instance().GetFrameTime();
                    if (pHealth->mCurrentDeathTime < pHealth->mDeathTime)
                    {
                        //Hollow::ScriptingManager::Instance().RunScript("OnPlayerDeath");
                        return; 
                    }
                    pHealth->mCurrentDeathTime = 0.0;
                    pHealth->mIsDying = false;

                    // Empty HP UI array
                    mPlayerHPUIIcons.clear();
					
					// Broadcast player death even
					Hollow::EventManager::Instance().BroadcastToSubscribers(PlayerDeathEvent());
					return;
				}

				// Send event to destroy object
				DeathEvent death(mGameObjects[i]->mType);
				death.mpObject1 = mGameObjects[i];
				Hollow::EventManager::Instance().BroadcastToSubscribers(death);
				Hollow::GameObjectManager::Instance().DeleteGameObject(mGameObjects[i]);
			}
			// Create new UI object
			if (mGameObjects[i]->mType == (int)GameObjectType::PLAYER && pHealth->mIsAlive)
			{
				// Populate UIIcons
				if (mPlayerHPUIIcons.empty())
				{
					HW_WARN("NO HP UI");
					int numUIIcons = pHealth->mHitPoints / 2;
					for (int i = 0; i < numUIIcons; ++i)
					{
						CreateHPUIIcon(i);
					}
				}
				else
				{
					int playerHealth = pHealth->mHitPoints;
					if (playerHealth > mPlayerHPUIIcons.size()*2)
					{
						CreateHPUIIcon(mPlayerHPUIIcons.size());
					}
					for (int UIIndex = 0; UIIndex < mPlayerHPUIIcons.size(); ++UIIndex)
					{
						Hollow::GameObject* pUIIcon = mPlayerHPUIIcons[UIIndex];
						Hollow::UIImage* pUIImg = pUIIcon->GetComponent<Hollow::UIImage>();
						// Play should have full hp
						if (playerHealth >= (UIIndex + 1)*2)
						{
							pUIImg->TexturePath = "Resources/Textures/UI/HPIcon.png";
						}
						else if (playerHealth <= (UIIndex) * 2)
						{
							pUIImg->TexturePath = "Resources/Textures/UI/EmptyHPIcon.png";
						}
						else
						{
							pUIImg->TexturePath = "Resources/Textures/UI/HalfHPIcon.png";
						 }
						pUIImg->mpTexture = Hollow::ResourceManager::Instance().LoadTexture(pUIImg->TexturePath);
					}
					//Hollow::UIImage* pUIImg = mpPlayerHPUI->GetComponent<Hollow::UIImage>();
					//pUIImg->TexturePath = "Resources/Textures/HalfHPIcon.png";
					//pUIImg->mpTexture = Hollow::ResourceManager::Instance().LoadTexture(pUIImg->TexturePath);
				}
			}
            // Update reaction to hit time
            if (pHealth->mIsHit)
            {
                pHealth->mCurrentHitReactionTime += Hollow::FrameRateController::Instance().GetFrameTime();

                // Check if should still be invincible
                if (pHealth->mCurrentHitReactionTime > pHealth->mHitReactionTime)
                {
                    pHealth->mIsHit = false;
                    pHealth->mCurrentHitReactionTime = 0.0;
                }
            }
		}
	}

	void HealthSystem::AddGameObject(Hollow::GameObject* pGameObject)
	{
		CheckAllComponents<Health>(pGameObject);
	}

	void HealthSystem::HandleBroadcastEvent(Hollow::GameEvent& event)
	{
	}

	void HealthSystem::SubscribeToEvents()
	{
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_BULLET_HIT_PLAYER, EVENT_CALLBACK(HealthSystem::OnBulletHitPlayer));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_BULLET_HIT_WALL, EVENT_CALLBACK(HealthSystem::OnBulletHitWall));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_BULLET_HIT_DOOR, EVENT_CALLBACK(HealthSystem::OnBulletHitDoor));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_BULLET_HIT_FLOOR, EVENT_CALLBACK(HealthSystem::OnBulletHitFloor));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_PLAYER_BULLET_HIT_ENEMY, EVENT_CALLBACK(HealthSystem::OnPlayerBulletHitEnemy));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::FLOOR_CLEARED_DELAYED, EVENT_CALLBACK(HealthSystem::OnFloorCleared));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_ENEMY_AOE_DAMAGE_HIT_PLAYER, EVENT_CALLBACK(HealthSystem::OnAOEDamageHitPlayer));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_PLAYER_AOE_HIT_ENEMY, EVENT_CALLBACK(HealthSystem::OnPlayerAOEHitEnemy));
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::ON_BULLET_HIT_DESTRUCTIBLE_WALL, EVENT_CALLBACK(HealthSystem::OnBulletHitDestructibleWall));
	}

	void HealthSystem::OnBulletHitPlayer(Hollow::GameEvent& event)
	{
		Hollow::GameObject* pPlayer = nullptr;
		if (event.mpObject1->mType == (int)GameObjectType::PLAYER)
		{
			// Call handle function with player, bullet
			HandleBulletDamage(event.mpObject1, event.mpObject2);
			pPlayer = event.mpObject1;
		}
		else
		{
			// Call handle function with input reversed
			HandleBulletDamage(event.mpObject2, event.mpObject1, true);
			pPlayer = event.mpObject2;
		}
			
		Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/PlayerHit.wav");
	}

	void HealthSystem::OnBulletHitWall(Hollow::GameEvent& event)
	{
		// Get bullet and wall object
		Hollow::GameObject* pWall = nullptr;
		Hollow::GameObject* pBullet = nullptr;
		if (event.mpObject1->mType == (int)GameObjectType::WALL)
		{
			pWall = event.mpObject1;
			pBullet = event.mpObject2;
		}
		else
		{
			pWall = event.mpObject2;
			pBullet = event.mpObject1;
		}

		if (pBullet->mTag == "FIRE")
		{
			Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/FireSpellDestroy.wav");
		}
		Hollow::GameObjectManager::Instance().DeleteGameObject(pBullet);

	}

    void HealthSystem::OnBulletHitDoor(Hollow::GameEvent& event)
    {
        if (event.mpObject1->mType == (int)GameObjectType::DOOR)
        {
            Hollow::GameObjectManager::Instance().DeleteGameObject(event.mpObject2);
        }
        else
        {
            Hollow::GameObjectManager::Instance().DeleteGameObject(event.mpObject1);
        }
    }

    void HealthSystem::OnBulletHitFloor(Hollow::GameEvent& event)
    {
        if (event.mpObject1->mType == (int)GameObjectType::FLOOR)
        {
            Hollow::GameObjectManager::Instance().DeleteGameObject(event.mpObject2);
        }
        else
        {
            Hollow::GameObjectManager::Instance().DeleteGameObject(event.mpObject1);
        }
    }

	void HealthSystem::OnPlayerBulletHitEnemy(Hollow::GameEvent& event)
	{		
		if (event.mpObject1->mType == (int)GameObjectType::ENEMY)
		{
			HandleBulletDamage(event.mpObject1, event.mpObject2, true);
		}
		else
		{
			HandleBulletDamage(event.mpObject2, event.mpObject1,true);
		}
		Hollow::AudioManager::Instance().PlayEffect("Resources/Audio/SFX/BossHit.wav");
	}

    void HealthSystem::OnFloorCleared(Hollow::GameEvent& event)
    {
        // Empty HP UI array
        mPlayerHPUIIcons.clear();
    }

	void HealthSystem::OnAOEDamageHitPlayer(Hollow::GameEvent& event)
	{
		Hollow::GameObject* gameobject;
		Hollow::GameObject* aoe;
		if (event.mpObject1->mType == (int)GameObjectType::PLAYER)
		{
			gameobject = event.mpObject1;
			aoe = event.mpObject2;
		}
		else
		{
			gameobject = event.mpObject2;
			aoe = event.mpObject1;
		}
        HandleBulletDamage(gameobject, aoe, false);

        if (aoe->mTag == "AOE_APPLIES_IMPULSE")
        {
            glm::vec3 impulse = glm::vec3(0.0f);
            glm::vec3 aoe_pos = aoe->GetComponent<Hollow::Transform>()->mPosition;
            glm::vec3 player_pos = gameobject->GetComponent<Hollow::Transform>()->mPosition;
            glm::vec3 direction = glm::normalize(player_pos - aoe_pos);
            impulse = direction * 100.0f;
            Hollow::PhysicsManager::Instance().ApplyLinearImpulse(gameobject, impulse);
        }
	}

	void HealthSystem::OnPlayerAOEHitEnemy(Hollow::GameEvent& event)
	{
		Hollow::GameObject* gameobject;
		Hollow::GameObject* aoe;
		if (event.mpObject1->mType == (int)GameObjectType::ENEMY)
		{
			gameobject = event.mpObject1;
			aoe = event.mpObject2;
		}
		else
		{
			gameobject = event.mpObject2;
			aoe = event.mpObject1;
		}
        HandleBulletDamage(gameobject, aoe, false);
        if (aoe->mTag != "AIREARTH")
        {
            glm::vec3 impulse = glm::vec3(0.0f);
            glm::vec3 aoe_pos = aoe->GetComponent<Hollow::Transform>()->mPosition;
            glm::vec3 player_pos = gameobject->GetComponent<Hollow::Transform>()->mPosition;
            glm::vec3 direction = glm::normalize(player_pos - aoe_pos);
            impulse = direction * 50.0f;
            Hollow::PhysicsManager::Instance().ApplyLinearImpulse(gameobject, impulse);
        }
	}

	void HealthSystem::OnBulletHitDestructibleWall(Hollow::GameEvent& event)
	{
		Hollow::GameObject* pDWall = nullptr;

		if (event.mpObject1->mType == (int)GameObjectType::DESTRUCTIBLE_WALL)
		{
			Hollow::GameObjectManager::Instance().DeleteGameObject(event.mpObject2);
			pDWall = event.mpObject1;
		}
		else
		{
			Hollow::GameObjectManager::Instance().DeleteGameObject(event.mpObject1);
			pDWall = event.mpObject2;
		}

		// Damage the wall
		Health* pWallHealth = pDWall->GetComponent<Health>();
        TakeDamage(pWallHealth, 2, 3);
	}
    void HealthSystem::OnEnemyAoeHitPlayer(Hollow::GameEvent& event)
    {
        Hollow::GameObject* player = nullptr;

        if (event.mpObject1->mType == (int)GameObjectType::PLAYER)
        {
            player = event.mpObject1;
        }
        else
        {
            player = event.mpObject2;
        }

        // Damage the wall
        Health* pPlayerHealth = player->GetComponent<Health>();
        TakeDamage(pPlayerHealth, 1, 3);
    }

	void HealthSystem::HandleBulletDamage(Hollow::GameObject* pObjectHit, Hollow::GameObject* pBullet, bool isBulletDestructible)
	{
		// Destroy player bullet
        if (isBulletDestructible)
        {
            Hollow::GameObjectManager::Instance().DeleteGameObject(pBullet);
        }

		int damage = mMapBulletDamage[pBullet->mTag];
		
		CharacterStats* pStats = pObjectHit->GetComponent<CharacterStats>();

		if (pStats)
		{
			damage *= pStats->mDamageFactor;
		}

		// Decrease player health, object hit must have a health component
		Health* pHealth = pObjectHit->GetComponent<Health>();
        float invincibilityTime = 0.5f;
        if (pObjectHit->mType == (int)GameObjectType::PLAYER)
        {
            invincibilityTime = 3.0f;
        }
        TakeDamage(pHealth, damage, invincibilityTime);
	}

	void HealthSystem::CreateHPUIIcon(int index)
	{
		const int size = 80;
		glm::ivec2 screenSize = Hollow::RenderManager::Instance().GetWindowSize();
		
		Hollow::GameObject* pUIObj = Hollow::ResourceManager::Instance().LoadGameObjectFromFile("Resources/Prefabs/UIIcon.json");
		Hollow::UIImage* pUIImg = pUIObj->GetComponent<Hollow::UIImage>();
		pUIImg->TexturePath = "Resources/Textures/UI/HPIcon.png";
		pUIImg->mpTexture = Hollow::ResourceManager::Instance().LoadTexture(pUIImg->TexturePath);

		Hollow::UITransform* pUITr = pUIObj->GetComponent<Hollow::UITransform>();
		pUITr->mScale = glm::vec2(size, size);
		pUITr->mPosition = glm::vec2(size * (index + 1), screenSize.y * 0.95);
		mPlayerHPUIIcons.push_back(pUIObj);
	}

    void HealthSystem::TakeDamage(Health* target, int damageTaken, float invincibilityTime)
    {
        if (target && target->mInvincible) 
        { 
            return; 
        }

        target->mHitPoints -= damageTaken;
        target->mInvincible = true;
        target->mInvincibleTime = target->mInvincibleTime;
        target->mIsHit = true;
    }

}
