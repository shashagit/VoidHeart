#include <hollowpch.h>
#include "Light.h"

#include "Hollow/Graphics/FrameBuffer.h"

namespace Hollow
{
	Light Light::instance;

	void Light::Init()
	{
	}

	void Light::Serialize(rapidjson::Value::Object data)
	{
		if (data.HasMember("Color"))
		{
			mColor = JSONHelper::GetVec3F(data["Color"].GetArray());
		}
		if (data.HasMember("CastShadow"))
		{
			mCastShadow = data["CastShadow"].GetBool();
			if (mCastShadow)
			{
				if (data.HasMember("ShadowMapSize"))
				{
					mShadowMapSize = JSONHelper::GetVec2F(data["ShadowMapSize"].GetArray());
					mpShadowMap = new FrameBuffer(mShadowMapSize.x, mShadowMapSize.y, 1, true);
				}
			}
		}
		
	}

	void Light::DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		JSONHelper::Write<glm::vec3>("Color", mColor, writer);
		JSONHelper::Write<bool>("CastShadow", mCastShadow, writer);
		if (mCastShadow)
		{
			JSONHelper::Write<glm::vec2>("ShadowMapSize", mShadowMapSize, writer);
		}
		JSONHelper::Write<float>("Radius", mRadius, writer);
	}

	void Light::Clear()
	{
	}

	void Light::DebugDisplay()
	{
	}
}