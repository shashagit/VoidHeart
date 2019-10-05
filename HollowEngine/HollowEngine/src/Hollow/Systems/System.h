#pragma once
#include <vector>
#include "Hollow/Managers/SystemManager.h"
#include "Hollow/Core/GameObject.h"


namespace Hollow
{
	class HOLLOW_API System
	{
	public:
		System(System* instance,int tier) {
			SystemManager::Instance().RegisterSystem(instance);
			this->mTier = tier;
		}
		virtual ~System() {}
		virtual void Update(GameObject* gameobject) = 0;
		virtual void AddGameObject(GameObject* pGameObject) = 0;

		void DoUpdate()
		{
			for (unsigned int i = 0; i < mGameObjects.size(); ++i)
			{
				Update(mGameObjects[i]);
			}
		}

		void DeleteGameObject(GameObject* pGameObject)
		{
			mGameObjects.erase(std::find(mGameObjects.begin(), mGameObjects.end(), pGameObject));
		}

		void DeleteAllGameObjects() 
		{
			mGameObjects.clear();
		}
	protected:
		template<typename First> // 1 template parameter
		void CheckComponents(GameObject* pGameObject)
		{
			if (pGameObject->GetComponent<First>())
			{
				mGameObjects.push_back(pGameObject);
			}
		}

		template<typename First, typename Second, typename ... Rest> // >=2 template parameters
		void CheckComponents(GameObject* pGameObject)
		{
			if (pGameObject->GetComponent<First>())
			{
				CheckComponents<Second, Rest...>(pGameObject);
			}
		}

	public:
		int mTier;

	protected:
		std::vector<GameObject*> mGameObjects;
	};
}