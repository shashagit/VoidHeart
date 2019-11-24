#pragma once
#include "Hollow/Common.h"

namespace Hollow
{
	class SceneManager
	{
		SINGLETON(SceneManager);
	public:
		HOLLOW_API void Init();
		HOLLOW_API void CleanUp();
		HOLLOW_API void DebugDisplay();

		HOLLOW_API void LoadLevel(std::string LevelFile);

	private:
		void DeserializeGameObject();
		void DeserializeLevel();
	private:
		// Creating Prefab
		std::string selectedComponent;
		char charBuffer[255];
		GameObject* selectedGameObject;
	};
}
