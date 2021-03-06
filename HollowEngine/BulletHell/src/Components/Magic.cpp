#include "Magic.h"

// TODO: Find a better place to put SpellType enum
// Coupling components this closely could cause issues
#include "Spell.h"

namespace BulletHell
{
	Magic Magic::instance;

	// Create all basic spells for storage in the map
	// Map is [int, Spell*] where int is the bit mask for the spell type
	std::unordered_map<int, Magic::SpellData*> Magic::mBasicSpells = Magic::CreateBasicSpellMap();

	// Create all combined spells for storage in the map
	// Map is [int, Spell*] where int is the bit mask for the spell types
	std::unordered_map<int, Magic::SpellData*> Magic::mCombinedSpells = Magic::CreateCombinedSpellMap();

	void Magic::Init()
	{	
	}

	void Magic::Serialize(rapidjson::Value::Object data)
	{
		if (data.HasMember("LeftHandScript"))
		{
			mLeftHandScriptPath = data["LeftHandScript"].GetString();
		}
		if (data.HasMember("RightHandScript"))
		{
			mRightHandScriptPath = data["RightHandScript"].GetString();
		}
		if (data.HasMember("CombineHandScript"))
		{
			mCombineHandScriptPath = data["CombineHandScript"].GetString();
		}
	}

	void Magic::DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		Hollow::JSONHelper::Write<std::string>("LeftHandScript", mLeftHandScriptPath, writer);
		Hollow::JSONHelper::Write<std::string>("RightHandScript", mRightHandScriptPath, writer);
		Hollow::JSONHelper::Write<std::string>("CombineHandScript", mCombineHandScriptPath, writer);
	}

	void Magic::Clear()
	{
		
	}

	void Magic::DebugDisplay()
	{
		Hollow::ImGuiHelper::InputText("Left Hand Path", mLeftHandScriptPath);
		Hollow::ImGuiHelper::InputText("Right Hand Path", mRightHandScriptPath);
		Hollow::ImGuiHelper::InputText("Combine Hand Path", mCombineHandScriptPath);

		if (mRightHandSpell != nullptr)
		{
			ImGui::Text("Right Hand Spell %s", mRightHandSpell->mName.c_str());
		}
		if (mLeftHandSpell != nullptr)
		{
			ImGui::Text("Left Hand Spell %s", mLeftHandSpell->mName.c_str());
		}
		if (mCombinedSpell != nullptr)
		{
			ImGui::Text("Combined Spell %s", mCombinedSpell->mName.c_str());
		}

		//ImGui::Columns(2, 0, true);
		//ImGui::Separator();
		//ImGui::Text("ID"); ImGui::NextColumn();
		for (auto& spell : mBasicSpells)
		{
			ImGui::Text("%s", spell.second->mName); ImGui::SameLine();
			ImGui::Text("Collected %i", spell.second->mCollected);
			ImGui::Text("RH CD: %f", spell.second->mRightHandCooldown); ImGui::SameLine();
			ImGui::Text("LH CD: %f", spell.second->mLeftHandCooldown);
		}

		ImGui::Text("Combined Spells");
		for (auto& spell : mCombinedSpells)
		{
			ImGui::Text("%s:", spell.second->mName); ImGui::SameLine();
			ImGui::Text("%f", spell.second->mCombinedCooldown);
		}
	}

	std::unordered_map<int, Magic::SpellData*> Magic::CreateBasicSpellMap()
	{
		std::unordered_map<int, Magic::SpellData*> spellMap;
		Magic::SpellData* pBasicSpell = new Magic::SpellData{ "Fire", "Spells/Fire", FIRE, 1.570796f, 0.0f, "Resources/Textures/fire.png", 0.5f, glm::vec3(0.6, 0.0, 0.0), "Resources/Textures/UI/Fire.png" };
		spellMap.emplace(SpellType::FIRE, pBasicSpell);

		pBasicSpell = new Magic::SpellData{ "Air", "Spells/Air", AIR, 3.14159f, 0.0f, "Resources/Textures/cracks.png", 0.5f, glm::vec3(0.4, 0.4, 0.4), "Resources/Textures/UI/Air.png" };
		spellMap.emplace(SpellType::AIR, pBasicSpell);

		pBasicSpell = new Magic::SpellData{ "Earth", "Spells/Earth", EARTH, 0.0f, 0.0f, "Resources/Textures/rocks.png" , 1.0f, glm::vec3(0.4, 0.2, 0.0), "Resources/Textures/UI/Earth.png" };
		spellMap.emplace(SpellType::EARTH, pBasicSpell);

		pBasicSpell = new Magic::SpellData{ "Water", "Spells/Water", WATER, 4.712388f, 0.0f, "Resources/Textures/water.png" , 0.75f, glm::vec3(0.1, 0.1, 0.7), "Resources/Textures/UI/Water.png" };
		spellMap.emplace(SpellType::WATER, pBasicSpell);

		return spellMap;
	}

	std::unordered_map<int, Magic::SpellData* > Magic::CreateCombinedSpellMap()
	{
		std::unordered_map<int, Magic::SpellData*> spellMap;
		Magic::SpellData* pCombinedSpell = new Magic::SpellData{ "Fire + Fire", "Spells/FireFire", COMBINED,  0.0f, 0.0f, "", 3.0f, glm::vec3(0.0f), "Resources/Textures/UI/FireFire.png" };
		spellMap.emplace(SpellType::FIRE | SpellType::FIRE, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Air + Air", "Spells/AirAir", COMBINED,  0.0f, 0.0f, "", 3.0f, glm::vec3(0.0f), "Resources/Textures/UI/AirAir.png" };
		spellMap.emplace(SpellType::AIR | SpellType::AIR, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Earth + Earth", "Spells/EarthEarth", COMBINED, 0.0f, 0.0f, "", 4.0f, glm::vec3(0.0f), "Resources/Textures/UI/EarthEarth.png" };
		spellMap.emplace(SpellType::EARTH | SpellType::EARTH, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Water + Water", "Spells/WaterWater", COMBINED, 0.0f, 0.0f, "", 10.0f, glm::vec3(0.0f), "Resources/Textures/UI/WaterWater.png" };
		spellMap.emplace(SpellType::WATER | SpellType::WATER, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Fire + Air", "Spells/FireAir", COMBINED,  0.0f, 0.0f, "", 0.1f, glm::vec3(0.0f), "Resources/Textures/UI/FireAir.png" };
		spellMap.emplace(SpellType::FIRE | SpellType::AIR, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Fire + Earth", "Spells/FireEarth", COMBINED, 0.0f, 0.0f, "", 12.0f, glm::vec3(0.0f), "Resources/Textures/UI/FireEarth.png" };
		spellMap.emplace(SpellType::FIRE | SpellType::EARTH, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Fire + Water", "Spells/FireWater", COMBINED, 0.0f, 0.0f, "", 1.0f, glm::vec3(0.0f), "Resources/Textures/UI/FireWater.png" };
		spellMap.emplace(SpellType::FIRE | SpellType::WATER, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Air + Earth", "Spells/AirEarth", COMBINED, 0.0f, 0.0f, "", 1.0f, glm::vec3(0.0f), "Resources/Textures/UI/AirEarth.png" };
		spellMap.emplace(SpellType::AIR | SpellType::EARTH, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Air + Water", "Spells/AirWater", COMBINED, 0.0f, 0.0f, "", 1.0f, glm::vec3(0.0f), "Resources/Textures/UI/AirWater.png" };
		spellMap.emplace(SpellType::AIR | SpellType::WATER, pCombinedSpell);

		pCombinedSpell = new Magic::SpellData{ "Earth + Water", "Spells/EarthWater", COMBINED, 0.0f, 0.0f, "", 1.0f, glm::vec3(0.0f), "Resources/Textures/UI/EarthWater.png" };
		spellMap.emplace(SpellType::EARTH | SpellType::WATER, pCombinedSpell);

		return spellMap;
	}	
}
