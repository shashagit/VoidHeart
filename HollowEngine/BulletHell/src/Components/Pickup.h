#pragma once
#include "Hollow.h"
#include "Hollow/Components/Component.h"

namespace BulletHell
{
	enum PickupType
	{
		HP,
		DASH,
		RATE_OF_FIRE,
		DAMAGE_FACTOR,
		SPEED,
		SPEED_FACTOR,
		INVINCIBILITY
	};

	class Pickup : public Hollow::Component
	{
		REGISTERCOMPONENT(Pickup);
	public:
		void Init();
		void Serialize(rapidjson::Value::Object data);
		void DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer);
		void Clear();
		void DebugDisplay();

	public:
		int mPickupType;

		// this gets added to the stat getting buffed
		float mBuffValue;

		// if timed effect
		float mEffectTime;
	};
}
