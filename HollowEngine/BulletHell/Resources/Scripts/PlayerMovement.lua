impulse = vec3.new()
direction = vec3.new();

front = vec3.new(0.0, 0.0, -1.0)
right = vec3.new(1.0, 0.0, 0.0)

front.y = 0.0
right.y = 0.0

speed = vec3.new(1000.0, 0.0, 1000.0)

if IsKeyPressed("W") or GetAxis(CONTROLLER["LY"]) < -16000 then
	impulse = impulse + front;
	direction = direction + vec3.new(0.0, 0.0, -1.0)
end
if IsKeyPressed("S") or GetAxis(CONTROLLER["LY"]) > 16000 then
	impulse = impulse - front;
	direction = direction + vec3.new(0.0, 0.0, 1.0)
end
if IsKeyPressed("A") or GetAxis(CONTROLLER["LX"]) < -16000 then
	impulse = impulse - right;
	direction = direction + vec3.new(-1.0, 0.0, 0.0)
end
if IsKeyPressed("D") or GetAxis(CONTROLLER["LX"]) > 16000 then
	impulse = impulse + right;
	direction = direction + vec3.new(1.0, 0.0, 0.0)
end
if IsKeyPressed("SPACE") or IsControllerButtonTriggered(CONTROLLER["A"]) then
	impulse = impulse + vec3.new(0.0, 20.0, 0.0);
end

impulse = impulse * speed;

ApplyLinearImpulse(gameObject, impulse)

body = gameObject:GetBody()

-- Damp Overall Velocity and Rotation
body.velocity = body.velocity - 0.8 * body.velocity 
body.angularVelocity = body.angularVelocity - 0.8* body.angularVelocity;
