#include "Health.h"
#include "Hollow/Managers/ScriptingManager.h"

namespace BulletHell
{
	Health Health::instance;

	void Health::Init()
	{
        // Register to Lua
        auto& lua = Hollow::ScriptingManager::Instance().lua;
        lua.new_usertype<Health>("Health",
            sol::constructors<Health()>(),
            "hitPoints", &Health::mHitPoints,
            "isHit", & Health::mIsHit,
            "currentHitReactionTime", &Health::mCurrentHitReactionTime,
            "hitReactionTime", &Health::mHitReactionTime,
            "currentDeathTime", & Health::mCurrentDeathTime,
            "deathTime", & Health::mDeathTime,
            "isDying", & Health::mIsDying,
			"invincibile", &Health::mInvincible,
			"currentInvincibiltyTime", &Health::mCurrentInvincibleTime
			);

        Hollow::ScriptingManager::Instance().mGameObjectType["GetHealth"] = &Hollow::GameObject::GetComponent<Health>;
	}

	void Health::Serialize(rapidjson::Value::Object data)
	{
		if (data.HasMember("HitPoints"))
		{
			mHitPoints = data["HitPoints"].GetInt();
		}
		if (data.HasMember("IsAlive"))
		{
			mIsAlive = data["IsAlive"].GetBool();
		}
		if (data.HasMember("Invincible"))
		{
			mInvincible = data["Invincible"].GetBool();
		}
		if (data.HasMember("InvincibleTime"))
		{
			mInvincibleTime = data["InvincibleTime"].GetFloat();
		}
        if (data.HasMember("IsHit"))
        {
            mIsHit = data["IsHit"].GetBool();
        }
        if (data.HasMember("HitReactinTime"))
        {
            mHitReactionTime = data["HitReactinTime"].GetFloat();
        }
        if (data.HasMember("DeathTime"))
        {
            mDeathTime = data["DeathTime"].GetFloat();
        }
        if (data.HasMember("IsDying"))
        {
            mIsDying = data["IsDying"].GetBool();
        }
	}

	void Health::DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		Hollow::JSONHelper::Write<int>("HitPoints", mHitPoints, writer);
		Hollow::JSONHelper::Write<bool>("IsAlive", mIsAlive, writer);
		Hollow::JSONHelper::Write<bool>("Invincible", mInvincible, writer);
		Hollow::JSONHelper::Write<float>("InvincibleTime", mInvincibleTime, writer);
        Hollow::JSONHelper::Write<bool>("IsHit", mIsHit, writer);
        Hollow::JSONHelper::Write<float>("HitReactionTime", mHitReactionTime, writer);
        Hollow::JSONHelper::Write<float>("DeathTime", mDeathTime, writer);
        Hollow::JSONHelper::Write<bool>("IsDying", mIsDying, writer);
	}

	void Health::Clear()
	{
	}

	void Health::DebugDisplay()
	{
		ImGui::InputInt("Hit Points", &mHitPoints);
		ImGui::Checkbox("Is Alive", &mIsAlive);
		ImGui::Checkbox("Invincible", &mInvincible);
        ImGui::Checkbox("Is Dying", &mIsDying);
		ImGui::InputFloat("Invincible Time", &mInvincibleTime);
		ImGui::InputFloat("Current Invincible Timer", &mCurrentInvincibleTime);
        ImGui::InputFloat("Reaction Time To Hits", &mHitReactionTime);
        ImGui::InputFloat("Death Time To Hits", &mDeathTime);
	}

}