#include <hollowpch.h>
#include "UIText.h"

namespace Hollow
{
	UIText UIText::instance;
	
	void UIText::Init()
	{
		mOffsetPosition = glm::vec2(0.0f);
		mTextScale = glm::vec2(1.0f);
		mText = "";
		mColor = glm::vec3(0.0f);
	}

	void UIText::Serialize(rapidjson::Value::Object data)
	{
		if(data.HasMember("OffsetPosition"))
		{
			mOffsetPosition = JSONHelper::GetVec2F(data["OffsetPosition"].GetArray());
		}
		if(data.HasMember("TextScale"))
		{
			mTextScale = JSONHelper::GetVec2F(data["TextScale"].GetArray());
		}
		if(data.HasMember("Text"))
		{
			mText = data["Text"].GetString();
		}
		if(data.HasMember("Color"))
		{
			mColor = JSONHelper::GetVec3F(data["Color"].GetArray());
		}
	}

	void UIText::DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		JSONHelper::Write("OffsetPosition", mOffsetPosition, writer);
		JSONHelper::Write("TextScale", mTextScale, writer);
		JSONHelper::Write("Text", mText, writer);
		JSONHelper::Write("Color", mColor, writer);
	}

	void UIText::Clear()
	{
	}

	void UIText::DebugDisplay()
	{
		ImGui::InputFloat2("OffsetPosition", &mOffsetPosition[0]);
		ImGui::InputFloat2("TextScale", &mTextScale[0]);
		ImGuiHelper::InputText("Text", mText);
		ImGui::ColorEdit3("Color", &mColor[0], ImGuiColorEditFlags_Float);
	}
}