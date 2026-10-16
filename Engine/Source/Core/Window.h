#pragma once
#include <string>
#include "glm/fwd.hpp"
struct GLFWwindow;

namespace rv { 

enum class WindowMode;
struct ProjectSettings;

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
	bool maximized = true;

	WindowData() noexcept : title("Rvela Engine"), size(1920, 1080), maximized(true) {}
	WindowData(const std::string& title, int width, int height, bool maximized = true) noexcept
		: title(title), size(width, height), maximized(maximized) {}
};

class Window
{
public:
	Window() noexcept;
	Window(const WindowData& data) noexcept;
	Window(const std::string& title, int width, int height, bool maximized = true) noexcept;
	~Window();

	static Window Create() noexcept;
	static Window Create(const std::string& title, int width, int height, bool maximized = true) noexcept;
	void Init();

	GLFWwindow* GetGLFWWindow() const noexcept
	{
		return m_Window;
	}

	const std::string& GetTitle() const noexcept
	{
		return m_WindowData.title;
	}
	void SetTitle(const std::string& title);

	WindowSize GetSize() noexcept; 
	void SetSize(int width, int height);
	void SetPosition(int x, int y);
	void CenterOnMonitor();
	glm::ivec2 GetPosition() const;
	bool IsMaximized() const;
	void Maximize();
	void Restore();
	void Shutdown() const;

	void SetVSync(bool vsync);
	bool IsVSync() const noexcept { return m_VSync; }
	void SetResizable(bool resizable);
	void SetBorderless(bool borderless);
	void SetWindowMode(WindowMode mode);
	void ApplySettings(const ProjectSettings& settings);
	
private:
	GLFWwindow* m_Window{};
	WindowData m_WindowData;
	bool m_VSync = true;
	bool m_Borderless = false;
	bool m_Resizable = true;

	void SetCallbacks() noexcept;

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept;
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) noexcept;
	static void MouseMovedCallback(GLFWwindow* window, double xpos, double ypos) noexcept;
	static void MouseScrolledCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept;
	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) noexcept;
	static void FileDropCallback(GLFWwindow* window, int pathCount, const char* paths[]) noexcept;

};

}