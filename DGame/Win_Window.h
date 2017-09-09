#pragma once

#include "Common.h"

class Win_Window 
{
	WNDCLASS  windowClass;
	HWND      windowHandle;
	HINSTANCE hInstance;
	HGLRC     hrc;
	HDC       hdc;

	glm::mat4 projectionMatrix;
	glm::vec2 cursorPos;
	
	Win_Window(HINSTANCE hInstance) :
		hInstance(hInstance)
	{
		isRunning = true;
		projectionMatrix = glm::ortho(static_cast<float>(-WIDTH / 2.0f),
									  static_cast<float>(WIDTH / 2.0f),
									  static_cast<float>(HEIGHT / 2.0f),
									  static_cast<float>(-HEIGHT / 2.0f),
									  -1.0f, 1.0f);
	}

public:
	static Win_Window* GetInstance()
	{
		static Win_Window* window;
		if (window == nullptr)
		{
			//GetModuleHandle(0) for geting hInstance
			window = new Win_Window(GetModuleHandle(0));
		}

		return window;
	}

	Win_Window(Win_Window&) = delete;

	static std::unordered_map<WPARAM, bool> keyMap;
	//static bool leftMouseKeyPressed;
	//static bool rightMouseKeyPressed;

	bool isRunning;
	int drawWidth;
	int drawHeight;
	int xBias;
	int yBias;

	static LRESULT CALLBACK MainWinProc(HWND hWind, UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	void SetCursor(const glm::vec2 position) { cursorPos = position; }
	glm::vec2 GetCursor() 
	{
		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(windowHandle, &cursor);
		cursorPos.x = static_cast<float>(cursor.x);
		cursorPos.y = static_cast<float>(cursor.y);
		
		return cursorPos;
	}
	
	glm::vec2 GetCursorScreenPosition()
	{
		POINT cursor;
		GetCursorPos(&cursor);
		ScreenToClient(windowHandle, &cursor);
		cursorPos = glm::vec2(cursor.x, cursor.y);
		cursorPos -= glm::vec2(static_cast<float>(WIDTH / 2.0f), static_cast<float>(HEIGHT / 2.0f));
		
		glm::vec4 tmpCursor = projectionMatrix * glm::vec4(cursorPos, 0.0f, 1.0f);
		return glm::vec2(tmpCursor.x, tmpCursor.y);
	}

	glm::vec2 GetCursorNormalized() 
	{
		POINT tmpCursor;
		GetCursorPos(&tmpCursor);
		ScreenToClient(windowHandle, &tmpCursor);
		
		glm::vec2 center(WIDTH / 2.0f, HEIGHT / 2.0f);
		
		tmpCursor.x -= static_cast<LONG>(center.x);
		tmpCursor.y -= static_cast<LONG>(center.y);
		
		float cursorX = tmpCursor.x / center.x;
		float cursorY = tmpCursor.y / center.y;

		return glm::vec2(cursorX, cursorY);
	}
	void Createwindow();
	void SetUpOpenGL();

	const glm::mat4& GetProjection() { return projectionMatrix; }
	void SetProjection(const glm::mat4& projection) { projectionMatrix = projection; }


	void ShutDown() 
	{ 
		isRunning = false; 
		DestroyWindow(windowHandle);
		exit(EXIT_SUCCESS);
	}
	
	void Swapwindow() { SwapBuffers(hdc); }
	HWND GetWindowHandle() { return windowHandle; }
	void ShowError(const std::string& errorMessage, const std::string& errorTitle)
	{
		MessageBoxExA(windowHandle, errorMessage.c_str(), errorTitle.c_str(), MB_OK, 0);
		ShutDown();
	}
};



