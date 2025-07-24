#pragma once

#include "GLAD/gl.h"
#include "GLFW/glfw3.h"
#include <string>

struct WindowSize
{
	int width, height;

	WindowSize() noexcept :width(0), height(0) {}
	WindowSize(int width, int height) noexcept
		: width(width < 0 ? 0 : width), height(height < 0 ? 0 : height) {
	}
};

struct WindowData
{
	std::string title;
	WindowSize size;

	WindowData() noexcept : title("Rvela Engine"), size(1920,1080) {}
	WindowData(const std::string& title, int width, int height) noexcept : title(title), size(width,height) {}
};

class Window
{
public:
	Window() noexcept;
	Window(const std::string& title, int width,int height) noexcept;
	~Window();

	static Window Create() noexcept;
	static Window Create(const std::string& title, int width, int height) noexcept;
	void Init();

	GLFWwindow* GetGLFWWindow() const noexcept
	{
		return m_Window;
	}

	const std::string& GetTitle() const noexcept
	{
		return m_WindowData.title;
	}

	WindowSize GetSize() noexcept {
		if (!m_Window) {
			return WindowSize{ 0, 0 };
		}
		glfwGetWindowSize(m_Window, &m_WindowData.size.width, &m_WindowData.size.height);
		return m_WindowData.size;
	}

	inline void Shutdown() const
	{
		glfwDestroyWindow(m_Window);
	}

private:
	GLFWwindow* m_Window;
	WindowData m_WindowData;

	void SetCallbacks() noexcept;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
	static void MouseMovedCallback(GLFWwindow* window, double xpos, double ypos) noexcept;
	static void MouseScrolledCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept;
	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept;

};