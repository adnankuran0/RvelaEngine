#include "RuntimeBlitLayer.h"
#include "Core/Engine.h"
#include "Renderer/RenderLayer.h"
#include "Utils/ProjectManager.h"
#include "Input/Input.h"
#include <algorithm>
#include <cmath>

void rv::RuntimeBlitLayer::OnRender()
{
	int winWidth = 0, winHeight = 0;
	glfwGetFramebufferSize(m_Engine->GetWindow().GetGLFWWindow(), &winWidth, &winHeight);
	if (winWidth <= 0 || winHeight <= 0)
		return;

	StretchMode stretchMode = StretchMode::Viewport;
	StretchAspect stretchAspect = StretchAspect::Keep;
	uint32_t vpW = 1920;
	uint32_t vpH = 1080;

	if (ProjectManager::IsProjectLoaded())
	{
		const auto& settings = ProjectManager::GetActiveProject()->GetSettings();
		stretchMode = settings.stretchMode;
		stretchAspect = settings.stretchAspect;
		vpW = settings.viewportWidth > 0 ? settings.viewportWidth : 1920;
		vpH = settings.viewportHeight > 0 ? settings.viewportHeight : 1080;
	}

	uint32_t targetRenderW = vpW;
	uint32_t targetRenderH = vpH;

	if (stretchMode == StretchMode::Disabled || stretchMode == StretchMode::CanvasItems)
	{
		targetRenderW = static_cast<uint32_t>(winWidth);
		targetRenderH = static_cast<uint32_t>(winHeight);
	}
	else // Viewport
	{
		if (stretchAspect == StretchAspect::Expand)
		{
			float winAspect = static_cast<float>(winWidth) / static_cast<float>(winHeight);
			float vpAspect = static_cast<float>(vpW) / static_cast<float>(vpH);
			if (winAspect > vpAspect)
			{
				targetRenderW = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpH) * winAspect)));
				targetRenderH = vpH;
			}
			else
			{
				targetRenderW = vpW;
				targetRenderH = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpW) / winAspect)));
			}
		}
		else if (stretchAspect == StretchAspect::KeepWidth)
		{
			targetRenderW = vpW;
			float aspect = static_cast<float>(winWidth) / static_cast<float>(winHeight);
			targetRenderH = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpW) / aspect)));
		}
		else if (stretchAspect == StretchAspect::KeepHeight)
		{
			targetRenderH = vpH;
			float aspect = static_cast<float>(winWidth) / static_cast<float>(winHeight);
			targetRenderW = static_cast<uint32_t>(std::max(1.0f, std::round(static_cast<float>(vpH) * aspect)));
		}
		else
		{
			targetRenderW = vpW;
			targetRenderH = vpH;
		}
	}

	m_Engine->GetRenderLayer().SetTargetResolution(targetRenderW, targetRenderH);

	GLuint finalFBO = m_Engine->GetRenderLayer().GetFinalFramebuffer();
	auto& renderCtx = m_Engine->GetRenderLayer().GetRenderContext();
	uint32_t srcW = renderCtx.viewportWidth > 0 ? renderCtx.viewportWidth : targetRenderW;
	uint32_t srcH = renderCtx.viewportHeight > 0 ? renderCtx.viewportHeight : targetRenderH;

	int dstX0 = 0, dstY0 = 0, dstX1 = winWidth, dstY1 = winHeight;

	if (stretchMode == StretchMode::Disabled)
	{
		int centerX = winWidth / 2;
		int centerY = winHeight / 2;
		dstX0 = centerX - static_cast<int>(srcW) / 2;
		dstY0 = centerY - static_cast<int>(srcH) / 2;
		dstX1 = dstX0 + static_cast<int>(srcW);
		dstY1 = dstY0 + static_cast<int>(srcH);
	}
	else if (stretchMode == StretchMode::CanvasItems || stretchAspect == StretchAspect::Ignore || stretchAspect == StretchAspect::Expand)
	{
		dstX0 = 0;
		dstY0 = 0;
		dstX1 = winWidth;
		dstY1 = winHeight;
	}
	else
	{
		float srcAspect = static_cast<float>(srcW) / static_cast<float>(srcH);
		float winAspect = static_cast<float>(winWidth) / static_cast<float>(winHeight);

		int targetW = winWidth;
		int targetH = winHeight;

		if (winAspect > srcAspect)
		{
			targetH = winHeight;
			targetW = static_cast<int>(std::round(static_cast<float>(winHeight) * srcAspect));
		}
		else
		{
			targetW = winWidth;
			targetH = static_cast<int>(std::round(static_cast<float>(winWidth) / srcAspect));
		}

		dstX0 = (winWidth - targetW) / 2;
		dstY0 = (winHeight - targetH) / 2;
		dstX1 = dstX0 + targetW;
		dstY1 = dstY0 + targetH;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(m_Engine->GetWindow().GetGLFWWindow(), &mouseX, &mouseY);
	int dispW = dstX1 - dstX0;
	int dispH = dstY1 - dstY0;
	if (dispW > 0 && dispH > 0)
	{
		float normX = (static_cast<float>(mouseX) - static_cast<float>(dstX0)) / static_cast<float>(dispW);
		float normY = (static_cast<float>(mouseY) - static_cast<float>(winHeight - dstY1)) / static_cast<float>(dispH);
		Input::SetViewportMousePos(glm::vec2(normX * static_cast<float>(srcW), normY * static_cast<float>(srcH)));
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, winWidth, winHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, finalFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	GLenum blitFilter = GL_NEAREST;
	if (ProjectManager::IsProjectLoaded())
	{
		if (ProjectManager::GetActiveProject()->GetSettings().scalingFilter == ScalingFilter::Linear)
			blitFilter = GL_LINEAR;
		else
			blitFilter = GL_NEAREST;
	}

	glBlitFramebuffer(
		0, 0, srcW, srcH,
		dstX0, dstY0, dstX1, dstY1,
		GL_COLOR_BUFFER_BIT,
		blitFilter
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
