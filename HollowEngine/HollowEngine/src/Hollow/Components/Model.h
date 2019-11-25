#pragma once
#include "Component.h"

namespace Hollow
{
	class Mesh;
	class Shader;
	enum Shapes;

	class Model : public Component
	{
		REGISTERCOMPONENT(Model);
	public:
		HOLLOW_API void Init() override;
		HOLLOW_API void Clear() override;
		HOLLOW_API void Serialize(rapidjson::Value::Object object) override;
		HOLLOW_API void DeSerialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) override;
		HOLLOW_API void DebugDisplay() override;

	private:
		std::string mShapeType;
		std::string mModelPath;

	public:
		std::vector<Mesh*> mMeshes;
		Shapes mShape;
		bool mCastShadow;
		bool mModelHasOffset;
	};
}