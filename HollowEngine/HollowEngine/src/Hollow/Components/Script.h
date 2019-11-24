#pragma once
#include "Component.h"

namespace Hollow {
	class Script : public Component
	{
		REGISTERCOMPONENT(Script);
	public:
		HOLLOW_API void Init() override;
		HOLLOW_API void DebugDisplay() override;
		HOLLOW_API void Clear() override;
		HOLLOW_API void Serialize(rapidjson::Value::Object data) override;
		HOLLOW_API void DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) override;

		std::string mScriptPath;
	};
}
