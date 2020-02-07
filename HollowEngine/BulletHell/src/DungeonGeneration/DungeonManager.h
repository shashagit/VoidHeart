#pragma once

//#include "Hollow/Common.h"
#include "DungeonFloor.h"
#include "Hollow/Utils/Singleton.h"
#include <Hollow.h>
#include <Hollow/Core/GameObject.h>


namespace BulletHell
{
    class DungeonManager
    {
    private: 
        DungeonManager();
    public: 
        DungeonManager(const DungeonManager&) = delete;
        DungeonManager& operator= (const DungeonManager&) = delete;
        DungeonManager(DungeonManager&&) = delete;
        DungeonManager& operator=(DungeonManager&&) = delete;
        static DungeonManager& Instance();
    public:
        void Init();
        void Generate();
        void Regenerate();
        void Construct();
        void ConfigureDungeon();
        void SetSeed(unsigned seed);
        unsigned GetSeed();

        std::vector<DungeonFloor> GetFloors() const;
        DungeonFloor& GetFloor(int index);
        void Print() const;

		void DebugDisplay();
    private:
        int length;
        int breadth;
        int numFloors;
        int firstFloorRoomCount;
    public:
		Hollow::GameObject* mpPlayerGo;
    private:
        std::vector<DungeonFloor> mFloors;
        unsigned mSeed;
    };
}
