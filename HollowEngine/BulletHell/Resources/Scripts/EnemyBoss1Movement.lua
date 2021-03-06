function FollowPlayer(isScared)
    -----------------------------------------
    -- playtesting vars
	local enemySpeed = 4.0
	local scareSpeed = 8.0
    local scareDistance = 10 -- distance at which enemy will run away
    -----------------------------------------
    local transform = gameObject:GetTransform()
	local position = transform.position

    --  acquire target 
    local target = player
    if (target == nil or target.isActive == false) then
        return
    end
    local targetTransform = target:GetTransform()
    local targetPos = targetTransform.position
    
    -- calculate direction
    local xDir = targetPos.x - position.x
	local zDir = targetPos.z - position.z
	local dirLength = math.sqrt(xDir*xDir + zDir*zDir)
	local xDirNorm = xDir / dirLength
	local zDirNorm = zDir / dirLength
	
    -- look at the target
    local body = gameObject:GetBody()
    local rot = vec3.new(0.0, 0.0, 0.0)
    local tangent = xDirNorm / zDirNorm
    local radians = math.atan(tangent)
    local degree = radians * 180 / math.pi
    if zDirNorm >= 0 then  
	    rot = vec3.new(0.0, degree, 0.0)
        body:RotateBody(rot)
    end
    if zDirNorm < 0 then 
	    rot = vec3.new(0.0, degree + 180, 0.0)
        body:RotateBody(rot)
    end
    
    -- setting the velocity
    body.velocity = vec3.new(xDirNorm, 0.0, zDirNorm)
	if (isScared == true) then 
        if (dirLength < scareDistance) then
            body.velocity.x = scareDistance * body.velocity.x * -1
            body.velocity.z = scareDistance * body.velocity.z * -1
        else
            body.velocity = vec3.new(0.0, 0.0, 0.0)
        end    
    else
        body.velocity = body.velocity * enemySpeed
    end

    -- collision handling
    if (dirLength < 2.0) then
        local impulseStrength = vec3.new(1000, 0, 1000)
        local impulse = vec3.new(xDirNorm, 0.0, zDirNorm)
        impulse  = impulse * impulseStrength
        ApplyLinearImpulse(player, impulse)

        local impulseReverse = vec3.new(-xDirNorm, 0.0, -zDirNorm)
        impulseReverse = impulseReverse * impulseStrength
        ApplyLinearImpulse(gameObject, impulseReverse)
    end
end

function MoveInCirle()
    -----------------------------------------
    -- playtesting vars
	local minDist = 6   -- keep the distance from player
    local velocity = 5  -- how fast too circle around player
    local scareDistance = 3 -- how fast boss rotates around player
    local circlingSpeed = 2
    local speedToCircle = 7
    -----------------------------------------

	local transform = gameObject:GetTransform()
	local selfPos = transform.position
	local body = gameObject:GetBody()

	-- Get Player
	local target = player
    if (target == nil or target.isActive == false) then
        return
    end
    local targetTransform = target:GetTransform()
    local targetPos = targetTransform.position

    -- calculate direction
    local xDir = targetPos.x - selfPos.x
	local zDir = targetPos.z - selfPos.z
	local dirLength = math.sqrt(xDir*xDir + zDir*zDir)
	local xDirNorm = xDir / dirLength
	local zDirNorm = zDir / dirLength
	
    -- look at the target
    local rot = vec3.new(0.0, 0.0, 0.0)
    local tangent = xDirNorm / zDirNorm
    local radians = math.atan(tangent)
    local degree = radians * 180 / math.pi
    if zDirNorm >= 0 then  
	    rot = vec3.new(0.0, degree, 0.0)
        body:RotateBody(rot)
    end
    if zDirNorm < 0 then 
	    rot = vec3.new(0.0, degree + 180, 0.0)
        body:RotateBody(rot)
    end

	-- Get Direction to move in circle
    local scale = transform.scale
	local playerVec = targetPos - selfPos
    transform.position.y = targetPos.y + scale.y /2
	local Y = vec3.new(0,1,0)
	local direction = VecNormalize(VecCross(Y,playerVec));
	-- if enemy collides with wall reverse direction VecCross(Y,playerVec)
	-- need event for WALL ENEMY collision

	-- Get Direction to move forward or backward
	local len = VecLength(playerVec)
	if (len < minDist-0.5) then
		direction = VecNormalize(direction + VecNormalize(vec3.new(-playerVec.x,-playerVec.y,-playerVec.z)))
	    velocity = speedToCircle
    elseif (len > minDist+0.5) then
		direction = VecNormalize(direction + VecNormalize(playerVec))
        velocity = speedToCircle
	else
        direction = direction * circlingSpeed
        velocity = circlingSpeed
    end

	-- Move the player
	
	body.velocity = direction * velocity
	--body.position = body.position + direction * velocity
	--transform.position = body.position
end

function Update()
    local maxHealth = 200; 
	local health = gameObject:GetHealth()
    local hitPoints = health.hitPoints
    if (hitPoints < maxHealth / 3) then
        FollowPlayer(true)
    elseif (hitPoints < maxHealth * 2 / 3) then
        MoveInCirle()
    else
        FollowPlayer(false)
    end
end

Update()