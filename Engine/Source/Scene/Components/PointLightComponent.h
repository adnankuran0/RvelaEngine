#pragma once
#include "Scene/Component.h"
#include "glm/glm.hpp"
#include "../nlohmann/json.hpp"

using json = nlohmann::json;


class PointLightComponent : public Component
{
public:

    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;
    float radius = 10.0f;
    bool castShadows = true;
    PointLightComponent()
    {
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    PointLightComponent(const glm::vec3& color, float intensity, float radius) : color(color), intensity(intensity), radius(radius) 
    {
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::string Serialize() const override
    {
        json j;
        j["color"] = { color.r,color.g,color.b };
        j["intensity"] = intensity;
        j["radius"] = radius;
        return j.dump(4);
    }

    void Deserialize(const std::string& jsonStr) override
    {
        json j = json::parse(jsonStr);
        auto colorData = j["color"];
        color = glm::vec3(colorData[0], colorData[1], colorData[2]);
        intensity = j["intensity"];
        radius = j["radius"];
    }
    GLuint GetShadowMap() { return shadowMap; }
    GLuint GetFramebuffer() { return fbo; }

    const int SHADOW_WIDTH = 4096;
    const int SHADOW_HEIGHT = 4096;

private:
    GLuint shadowMap;
    GLuint fbo;
};