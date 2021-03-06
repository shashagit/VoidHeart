-- Globals
currentFloor = 0
currentRoom = 4

-- Dungeon Configuration
firstFloorRoomCount = 1
numFloors = 1
dungeonLength = 1
dungeonBreadth = 1

-- main menu Room Configuration
roomSize = 20
wallLength = 8
wallHeight = 5
wallThickness = 1
doorWidth = 3
doorHeight = 2
doorThickness = 3

-- Enemy
firstFloorEnemyCount = 0

-- Cheat codes
gameMuted = false
playerInvincible = false

-- Utils
function GetRoomCenterPosition()
	local coords = GetDungeonFloor(currentFloor):GetRoomFromIndex(currentRoom):GetCoords()
	local pos = vec3.new(coords.y * roomSize + roomSize/2, 1, coords.x * roomSize + roomSize/2)
	return pos
end

-- Slideshow Global
slideShowTimer = 0
slideNum = 1

-- Water Boss Globals
waterBossZOffset = 0
waterBossTheta = 0
waterBossTheta2 = 0
waterBossRadiusTheta = 0
waterBossRadiusTheta2 = 0
waterBossAttackTime = 0
waterBossCircle = 1
waterBossCircleCenter = vec3.new(0,0,0)
waterBossCircleSelected = true
waterBossMoveDirection = true
waterBossSpawnSpell = false


-- AirAir Globals
isScaled = false

-- Earth Boss Globals
earthBossCharging = false

-- Fire Boss Globals
fireBossFlamethowerTimer = 0.0
