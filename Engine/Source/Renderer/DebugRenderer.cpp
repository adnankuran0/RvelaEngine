#include "rvelapch.h"
#include "DebugRenderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void DebugRenderer::Init()
{
	m_LineVAO.Init();
	m_LineVAO.Bind();
	m_LineVBO.Init();
	m_LineVBO.Bind();

	BufferLayout layout;
	layout.BindVertexBuffer(m_LineVBO.getID());
	layout.Push<float>(3);
	layout.Push<float>(4);
	
	m_LineVAO.SetBufferLayout(layout);

}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& color)
{
	m_LineVertices.emplace_back(from, color);
	m_LineVertices.emplace_back(to, color);
}

void DebugRenderer::DrawTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec4& color)
{
	DrawLine(v1, v2, color);
	DrawLine(v2, v3, color);
	DrawLine(v1, v3, color);
}

void DebugRenderer::DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color)
{
	// bottom
	DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
	DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
	DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z), color);
	DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z), color);

	// top
	DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
	DrawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
	DrawLine(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color);
	DrawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z), color);

	// vertical
	DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
	DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
	DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);
	DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);
}

void DebugRenderer::DrawFrustum(const glm::mat4& invViewProj, const glm::vec4& color)
{
	const glm::vec4 ndcCorners[8] = {
		// Near plane
		{ -1.0f, -1.0f, -1.0f, 1.0f },
		{  1.0f, -1.0f, -1.0f, 1.0f },
		{  1.0f,  1.0f, -1.0f, 1.0f },
		{ -1.0f,  1.0f, -1.0f, 1.0f },
		// Far plane
		{ -1.0f, -1.0f,  1.0f, 1.0f },
		{  1.0f, -1.0f,  1.0f, 1.0f },
		{  1.0f,  1.0f,  1.0f, 1.0f },
		{ -1.0f,  1.0f,  1.0f, 1.0f }
	};

	glm::vec3 pts[8];
	for (int i = 0; i < 8; ++i) {
		glm::vec4 p = invViewProj * ndcCorners[i];
		pts[i] = glm::vec3(p) / p.w;
	}

	// Near plane
	DrawLine(pts[0], pts[1], color);
	DrawLine(pts[1], pts[2], color);
	DrawLine(pts[2], pts[3], color);
	DrawLine(pts[3], pts[0], color);

	// Far plane
	DrawLine(pts[4], pts[5], color);
	DrawLine(pts[5], pts[6], color);
	DrawLine(pts[6], pts[7], color);
	DrawLine(pts[7], pts[4], color);

	DrawLine(pts[0], pts[4], color);
	DrawLine(pts[1], pts[5], color);
	DrawLine(pts[2], pts[6], color);
	DrawLine(pts[3], pts[7], color);
}

void DebugRenderer::BeginFrame()
{
	m_LineVertices.clear();
	m_TriangleVertices.clear();
}

void DebugRenderer::EndFrame(const glm::mat4& mvp)
{
	if (m_LineVertices.empty())
		return;

	glLineWidth(2.0f);

	m_LineVAO.Bind();
	m_LineVBO.Bind();
	m_LineVBO.Data(m_LineVertices.data(), m_LineVertices.size() * sizeof(DebugVertex), true);

	Shader& shader = ShaderManager::Get("Line");
	shader.use();
	shader.setMat4("u_ViewProjection", mvp);

	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_LineVertices.size()));

	m_LineVAO.Unbind();
	m_LineVBO.Unbind();
}