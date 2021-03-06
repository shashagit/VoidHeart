function CreateBeam()
	local go = CreatePrefabAtPosition("PlayerBeam", gameObject:GetTransform().position)
	local particle = gameObject:GetParticleEmitter()
	if particle then
		particle.active = false
	end
	PlaySFX("Resources/Audio/SFX/AirEarthSpellAttack.wav")
end

function CheckValidAttack()
	local attack = gameObject:GetAttack()
	if attack.shouldAttack then
		CreateBeam()
		--CastRay()
		attack.shouldAttack = false
	end
end

CheckValidAttack()