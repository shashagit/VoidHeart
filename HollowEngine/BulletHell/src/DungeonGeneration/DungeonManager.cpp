#include "DungeonManager.h"

#include "Hollow/Managers/ResourceManager.h"
#include "Hollow/Managers/ScriptingManager.h"
#include "Hollow/Managers/ImGuiManager.h"
#include "Hollow/Managers/EventManager.h"
#include "Hollow/Managers/GameObjectManager.h"

#include "Events/DeathEvent.h"

#include "Components/Attack.h"

#include "GameMetaData/GameEventType.h"

#include "Hollow/Components/Transform.h"
#include "Hollow/Components/Script.h"
#include "Hollow/Components/Body.h"
#include "Hollow/Components/Collider.h"

#include "GameLogic/GameLogicManager.h"

#include "Hollow/Core/GameObject.h"
#include "Hollow/Physics/Broadphase/Shape.h"

namespace BulletHell
{
	DungeonManager::DungeonManager() : mSeed(0) { HW_INFO("Initialized: DungeonManager"); SubscribeToEvents();
	}

    DungeonManager& DungeonManager::Instance()
    {
        static DungeonManager instance;
        return instance;
    }

    void DungeonManager::Init()
    {
        auto randomizer = Random::Range(0, MAXINT);
        while (!SetSeed(randomizer()));
        {
        }
        //SetSeed(1777913080);
        //mSeed = 521288629;
        //mSeed = 1993539175;
        
        Generate();
		
		// Add to ImGui display
		Hollow::ImGuiManager::Instance().AddDisplayFunction("Dungeon", std::bind(&DungeonManager::DebugDisplay, &DungeonManager::Instance()));
    }

    void DungeonManager::Generate()
    {

        for (int i = 0; i < numFloors; i++)
        {
            DungeonFloor dungeonFloor(length, breadth, i);
            int numRooms = firstFloorRoomCount + 2 * i;
            dungeonFloor.Generate(numRooms, mSeed + i);
            mFloors.push_back(dungeonFloor);
            //dungeonFloor.PrintFloor(1);
        }
        //system("PAUSE");
        // Construct the first floor
        Construct(0);
    }

    void DungeonManager::Regenerate()
    {
        for (DungeonFloor& dungeonFloor : mFloors)
        {
            dungeonFloor.ResetFloor();
        }
        mFloors.clear();

		// was this here to construct a new dungeon?
		// did Init have dungeon construction in it
        //GameLogicManager::Instance().Init();
    }

    bool DungeonManager::SetSeed(unsigned seed)
    {
        // check for bug seeds, investigate them later
        switch (seed)
        {
        //case ####: // blacklisted seed
        //    return false;
        //    break;
        default:
            mSeed = seed;
            return true;
        }
    }
	
    void DungeonManager::Construct(int floorIndex)
    {
        mFloors[floorIndex].ConstructFloor();
        mFloors[floorIndex].CreateMinimap();
    }

    void DungeonManager::ConfigureDungeon()
    {
    	// Set the configuration values read from last set globals into DungeonManager class
        auto& luaState = Hollow::ScriptingManager::Instance().lua;
        firstFloorRoomCount = luaState["firstFloorRoomCount"];
        numFloors = luaState["numFloors"];
        length = luaState["dungeonLength"];
        breadth = luaState["dungeonBreadth"];

        DungeonRoom::mWallLength = luaState["wallLength"];
        DungeonRoom::mWallHeight = luaState["wallHeight"];
        DungeonRoom::mWallThickness = luaState["wallThickness"];
        DungeonRoom::mRoomSize = luaState["roomSize"];
        DungeonRoom::mDoorWidth = luaState["doorWidth"];
        DungeonRoom::mDoorHeight = luaState["doorHeight"];
        DungeonRoom::mDoorThickness = luaState["doorThickness"];
    }

    unsigned DungeonManager::GetSeed()
    {
        return mSeed;
    }

    std::vector<DungeonFloor>& DungeonManager::GetFloors()
    {
        return mFloors;
    }

    DungeonFloor& DungeonManager::GetFloor(int floorNumber)
    {
        return mFloors[floorNumber];
    }

    void DungeonManager::Print() const
    {
        for (const DungeonFloor& floor : mFloors)
        {
            floor.PrintFloor();
        }
    }

    void DungeonManager::DebugDisplay()
	{
		ImGui::Text("Seed: %u", mSeed);
	}

	float DungeonManager::CastRay(const glm::vec3& start, const glm::vec3& dir)
	{
		Ray r{ start, glm::normalize(dir) };

		IntersectionData id, closest;
		closest.object = nullptr;
		closest.depth = std::numeric_limits<float>::max();

		// simple but inefficient solution
		for (int i = 0; i < Hollow::GameObjectManager::Instance().GetGameObjects().size(); ++i) {
			Hollow::GameObject* pGO = Hollow::GameObjectManager::Instance().GetGameObjects()[i];

			if (pGO->mType != (int)GameObjectType::WALL && pGO->mType != (int)GameObjectType::DOOR)
			{
				continue;
			}
			
			Hollow::Shape* shape = pGO->GetComponent<Hollow::Collider>()->mpShape;
			glm::mat3& rot = pGO->GetComponent<Hollow::Body>()->mRotationMatrix;
			glm::vec3 extents = pGO->GetComponent<Hollow::Transform>()->mScale;

			if (shape->TestRay(r, id, rot, extents)) {
				if (id.depth < closest.depth) {
					closest = id;
				}
			}
		}
		return closest.depth;
	}
	
    void DungeonManager::OnDeath(Hollow::GameEvent& event)
    {
        DeathEvent& pDeathEvent = dynamic_cast<DeathEvent&>(event);
        DungeonRoom& room = GetCurrentRoom();
    	
    	if(pDeathEvent.mType == (int)GameObjectType::ENEMY)
    	{
			room.mEnemies.erase(std::remove(room.mEnemies.begin(), room.mEnemies.end(), pDeathEvent.mpObject1));
            /*auto iter = std::find(room.mEnemies.begin(), room.mEnemies.end(), pDeathEvent.mpObject1);
    		if(iter != room.mEnemies.end())
    		{
                room.mEnemies.erase(iter);
    		}*/
    	}
    }

    DungeonRoom& DungeonManager::GetCurrentRoom()
    {
        auto& lua = Hollow::ScriptingManager::Instance().lua;
        int currentFloor = lua["currentFloor"].get<int>();
        int currentRoom = lua["currentRoom"].get<int>();
    	
        return mFloors[currentFloor].GetRoomFromIndex(currentRoom);
    }

	// Called from Lua
	// Fires an event in C++
	void DungeonManager::OnCurrentRoomUpdated(int current, int previousIndex)
	{
        auto& lua = Hollow::ScriptingManager::Instance().lua;
        int currentFloor = lua["currentFloor"].get<int>();
        mFloors[currentFloor].OnRoomEnter(current, previousIndex);
	}

    void DungeonManager::SubscribeToEvents()
	{
        Hollow::EventManager::Instance().SubscribeEvent((int)GameEventType::DEATH, EVENT_CALLBACK(DungeonManager::OnDeath));
	}

}
