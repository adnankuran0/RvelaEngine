#pragma once
#include <string>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <iostream>


struct WindowSize
{
	int width, height;

	WindowSize():width(0), height(0) {}
	WindowSize(int width, int height) :width(width), height(height) {}
};

struct WindowData
{
	std::string title;
	WindowSize size;

	WindowData(): title("Rvela Engine"), size(1280,720) {}
	WindowData(const std::string& title, int width, int height): title(title), size(width,height) {}
};

class Window
{
public:
	Window();
	Window(const std::string& title, int width,int height);
	~Window();

	static Window Create();
	static Window Create(const std::string& title, int width, int height);

	void Init();

	GLFWwindow* GetWindow() const;
	const std::string& GetTitle() const;
	WindowSize GetSize();

	void Shutdown() const;

private:
	GLFWwindow* m_Window;
	WindowData m_WindowData;
};