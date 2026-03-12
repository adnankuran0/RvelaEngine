#include "rvelapch.h"
#include "DebugRenderer.h"
#include "Rendering/ShaderManager.h"

using namespace rv;

void DebugRenderer::Init()
{
	m_LineVAO.Init();
	m_LineVAO.Bind();
	m_LineVBO.Init();
	m_LineVBO.Bind();

	BufferLayout layout;
	layout.BindVertexBuffer(m_LineVAO.getID());
	layout.Push<float>(3);
	layout.Push<float>(4);
	
	m_LineVAO.SetBufferLayout(layout);

	glLineWidth(2.0f);
}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	m_LineVertices.emplace_back(from, color);
	m_LineVertices.emplace_back(to, color);
}

void DebugRenderer::BeginFrame()
{
	m_LineVertices.clear();
	m_TriangleVertices.clear();
}

void DebugRenderer::EndFrame(const glm::mat4& mvp)
{
	m_LineVAO.Bind();
	m_LineVBO.Bind();

	m_LineVBO.Data(m_LineVertices.data(), m_LineVertices.size() * sizeof(DebugVertex), true);
	
	Shader& shader = ShaderManager::Get("Line");

	shader.use();
	shader.setMat4("uMVP",mvp);

	glDrawArrays(GL_LINES, 0, m_LineVertices.size());

	m_LineVAO.Unbind();
	m_LineVBO.Unbind();
}