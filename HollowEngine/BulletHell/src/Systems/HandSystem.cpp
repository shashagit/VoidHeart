#include <Hollow.h>
#include "HandSystem.h"

#include "Components/ParentOffset.h"
#include "Components/Magic.h"

#include "Events/CycleSpellEvent.h"

#include "Hollow/Components/Transform.h"
#include "Hollow/Components/UIImage.h"
#include "Hollow/Components/UITransform.h"
#include "Hollow/Components/ParticleEmitter.h"

#include "Hollow/Managers/ResourceManager.h"
#include "Hollow/Managers/EventManager.h"
#include "Hollow/Managers/InputManager.h"

#include "Hollow/Events/GameEvent.h"

#include "GameMetaData/GameEventType.h"
#include "GameMetaData/GameObjectType.h"

namespace BulletHell
{
	HandSystem HandSystem::instance;

	void HandSystem::Init()
	{
		// Subscribe to hand related events
		SubscribeToEvents();
	}

	void HandSystem::Update()
	{
		// Check if UI objects have been/should be created
		CheckHandUI();

		for (unsigned int i = 0; i < mGameObjects.size(); ++i)
		{
			//// TEMP
			Hollow::GameObject* pGameObject = mGameObjects[i];
			if (pGameObject->mType == (int)GameObjectType::HAND)
			{
				if (pGameObject->GetComponent<ParentOffset>()->mTag == "left")
				{
					mpLeftHand = pGameObject;
				}
				if (pGameObject->GetComponent<ParentOffset>()->mTag == "right")
				{
					mpRightHand = pGameObject;
				}
			}
			////

			// Update position based on offset and parent
			Hollow::Transform* pTr = mGameObjects[i]->GetComponent<Hollow::Transform>();
			ParentOffset* pParentOffset = mGameObjects[i]->GetComponent<ParentOffset>();
			Hollow::Transform* pParentTr = pParentOffset->mpParent->GetComponent<Hollow::Transform>();

			pTr->mPosition = pParentOffset->mOffset + pParentTr->mPosition;
		}
	}

	void HandSystem::AddGameObject(Hollow::GameObject* pGameObject)
	{
		CheckAllComponents<ParentOffset, Hollow::Transform>(pGameObject);
		if (pGameObject->mType == (int)GameObjectType::PLAYER)
		{
			mpPlayerObject = pGameObject;
		}
	}

	void HandSystem::HandleBroadcastEvent(Hollow::GameEvent& event)
	{
	}

	void HandSystem::CreateUIObjects()
	{
		const int UIScale = 228;
		// Create right hand UI and set position
		mpRightHandUI = CreateUIObject(glm::vec2(UIScale, UIScale), glm::vec2(1150, 128), "Resources/Textures/UIHightlight.png");

		// Create left hand UI and set position
		mpLeftHandUI = CreateUIObject(glm::vec2(UIScale, UIScale), glm::vec2(128, 128), "Resources/Textures/UIHightlight.png");

		// Create combined hand UI
		mpCombinedHandUI = CreateUIObject(glm::vec2(UIScale, UIScale), glm::vec2(600, 128), "Resources/Textures/UIHightlight.png");
	}

	void HandSystem::SubscribeToEvents()
	{
		Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::CYCLE_SPELL, EVENT_CALLBACK(HandSystem::CycleSpell));
	}

	Hollow::GameObject* HandSystem::CreateUIObject(const glm::vec2& scale, const glm::vec2& position, const std::string& texturePath)
	{
		Hollow::GameObject* UIObject = Hollow::ResourceManager::Instance().LoadGameObjectFromFile("Resources/Prefabs/UIIcon.json");
		// Set UI position
		Hollow::UITransform* pUITr = UIObject->GetComponent<Hollow::UITransform>();
		pUITr->mScale = scale;
		pUITr->mPosition = position;
		//pUITr->mRotation;

		// Set UI Texture
		Hollow::UIImage* pUIImage = UIObject->GetComponent<Hollow::UIImage>();
		pUIImage->TexturePath = texturePath;
		pUIImage->mpTexture = Hollow::ResourceManager::Instance().LoadTexture(texturePath);

		return UIObject;
	}

	void HandSystem::CheckHandUI()
	{
		if (mpLeftHandUI == nullptr && mpRightHandUI == nullptr && mpCombinedHandUI == nullptr)
		{
			CreateUIObjects();
		}
	}

	void HandSystem::CycleSpell(Hollow::GameEvent& event)
	{
		// Convert event to cycle spell event
		CycleSpellEvent& pCycleSpellEvent = dynamic_cast<CycleSpellEvent&>(event);
		
		// Get spell rotation value from the player object
		Hollow::GameObject* pHandUIObj = nullptr;
		Hollow::GameObject* pHandObj = nullptr;
		Magic::SpellData* pSpellData = nullptr;
		Magic* pMagic = mpPlayerObject->GetComponent<Magic>();
		if (pCycleSpellEvent.mHandString == "left")
		{
			pHandUIObj = mpLeftHandUI;
			pSpellData = pMagic->mLeftHandSpell;
			pHandObj = mpLeftHand;
		}
		else
		{
			pHandUIObj = mpRightHandUI;
			pSpellData = pMagic->mRightHandSpell;
			pHandObj = mpRightHand;
		}
		
		// Update rotation of the hightlight icon
		Hollow::UITransform* pUITr = pHandUIObj->GetComponent<Hollow::UITransform>();
		pUITr->mRotation = pSpellData->mUIRotation;
		//pUITr->mRotation += (3.1419f/2.0f);

		// Update hand particles
		Hollow::ParticleEmitter* pParticleEmitter = pHandObj->GetComponent<Hollow::ParticleEmitter>();
		pParticleEmitter->mDTexturePath = pSpellData->mParticleTexturePath;
		pParticleEmitter->mTexture = Hollow::ResourceManager::Instance().LoadTexture(pParticleEmitter->mDTexturePath);
	}

}