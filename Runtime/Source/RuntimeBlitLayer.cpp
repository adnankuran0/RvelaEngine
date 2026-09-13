#include "RuntimeBlitLayer.h"
#include "Core/Engine.h"
#include "Renderer/RenderLayer.h"

void rv::RuntimeBlitLayer::OnRender()
{
	GLuint finalFBO = m_Engine->GetRenderLayer().GetFinalFramebuffer();

	int width, height;
	glfwGetFramebufferSize(m_Engine->GetWindow().GetGLFWWindow(), &width, &height);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, finalFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(
		0, 0, 1920, 1080,
		0, 0, width, height,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
