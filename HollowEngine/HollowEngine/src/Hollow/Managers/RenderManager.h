#pragma once

#include "Utils/Singleton.h"
#include "Hollow/Graphics/RenderData.h"
#include <GL/glew.h>

namespace Hollow {

	class GameWindow;
	//class Camera;
	class Shader;
	class FrameBuffer;

	class HOLLOW_API RenderManager
	{
		SINGLETON(RenderManager)
	public:
		void Init(GameWindow* pWindow = nullptr);
		void CleanUp();
		void Update();
		//inline Camera* GetCamera(){ return mpCamera; }

	private:
		// Initialization Functions
		void InitializeGBuffer();

		void CreateDeferredShader();
		void CreateLocalLightShader();

		void CreateShadowMap(LightData& light);

		void GBufferPass();
		void GlobalLightingPass(LightData& light);
		void LocalLightingPass();

		void DrawAllRenderData(Shader* pShader);
		void DrawShadowCastingObjects(Shader* pShader);
		void DrawFSQ();
		void DrawSphere();

		void DrawParticles();

		void DrawDebugDrawings();

		void DrawShadowMap();

		// ImGui Debug functions
		void DebugDisplay();
		void DebugDisplayGBuffer();
		void DebugDisplayLighting();

	public:
		std::vector<RenderData> mRenderData;
		std::vector<DebugRenderData> mDebugRenderData;
		std::vector<LightData> mLightData;
		std::vector<ParticleData> mParticleData;
		std::vector<CameraData> mCameraData;

	private:
		// Transformation matricies
		glm::mat4 mProjectionMatrix;
		glm::mat4 mViewMatrix;

		GameWindow* mpWindow;
		//Camera* mpCamera;

		// Lighting
		Shader* mpDeferredShader;
		Shader* mpLocalLightShader;

		// Debug drawing Shader
		Shader* mpDebugShader;

		// G-Buffer
		Shader* mpGBufferShader;
		FrameBuffer* mpGBuffer;
		int mGBufferDisplayMode;

		// Shadows
		Shader* mpShadowMapShader;
		Shader* mpShadowDebugShader;
		bool mShadowMapDebugMode;
		unsigned int mShadowMapDebugLightIndex;

		// ParticleSystem
		Shader* mpParticleShader;
		bool ShowParticles;
	};
}