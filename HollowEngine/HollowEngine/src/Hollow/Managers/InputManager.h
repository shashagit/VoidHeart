#pragma once
#include "Hollow/Events/Event.h"

namespace Hollow {

	class InputManager
	{
		SINGLETON(InputManager)		
	public:
		HOLLOW_API bool IsKeyPressed(unsigned int keycode);
		HOLLOW_API bool IsKeyReleased(unsigned int keycode);
		HOLLOW_API bool IsKeyTriggered(unsigned int keycode);
		HOLLOW_API bool IsMouseButtonPressed(unsigned int button);
		HOLLOW_API bool IsMouseButtonTriggered(unsigned int button);

		HOLLOW_API glm::vec2 GetMouseMotion();
		
		HOLLOW_API glm::ivec2 GetMousePosition();
    
		HOLLOW_API float GetMouseX();
		HOLLOW_API float GetMouseY();

		using EventCallbackFn = std::function<void(Event&)>;

		HOLLOW_API void SetEventCallback(const EventCallbackFn& callback);
		
		HOLLOW_API void Init();
		HOLLOW_API void Update();
		HOLLOW_API void HideMouseCursor();
		HOLLOW_API void ShowMouseCursor();
		
	private:
		EventCallbackFn EventCallback;
		
		void HandleKeyboardEvents(const SDL_Event& e);
		void HandleMouseEvents(const SDL_Event& e);
		void HandleWindowEvents(const SDL_Event& e);

		float xRel, yRel;
		
		Uint8 mCurrentState[512];
		Uint8 mPreviousState[512];
		bool mPrevMouseState[6];
		bool mCurrentMouseState[6];
	};

}
