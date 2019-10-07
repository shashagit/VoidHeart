#include "PrototypeSystem.h"
#include "PrototypeComponent.h"

PrototypeSystem PrototypeSystem::instance;

void PrototypeSystem::Update(Hollow::GameObject* gameobject)
{
	//HW_TRACE("Prototype system has {0} gameobjects", mGameObjects.size());
}

void PrototypeSystem::AddGameObject(Hollow::GameObject* object)
{
	CheckComponents<PrototypeComponent>(object);
}
