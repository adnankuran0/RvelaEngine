#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "GLAD/gl.h"
#include "Utils/FileUtils.h"
#include "Shader.h"

namespace rv {

    class Skybox
    {
    public:
        Skybox();
        ~Skybox();

        void Init(const std::vector<Path>& faces);
        void InitHDR(const Path& hdrFilePath);

        void Render(const glm::mat4& projection, const glm::mat4& view, GLuint screenFBO);

        GLuint ConvertEquirectangularToCubemap(GLuint hdrTexture);
        GLuint LoadHDRTexture(const Path& hdrPath);

        inline GLuint GetEnvironmentMap() const { return skyboxTexture; }
        inline GLuint GetIrradianceMap() const { return irradianceMap; }
        inline GLuint GetPrefilterMap() const { return prefilterMap; }
        inline GLuint GetBRDFLUTTexture() const { return brdfLUTTexture; }

        inline bool HasIBLMaps() const { return irradianceMap != 0 && prefilterMap != 0 && brdfLUTTexture != 0; }
        inline int GetPrefilterMaxMipLevels() const { return prefilterMaxMipLevels; }
        Path& GetPath() { return m_Path; }
    private:
        GLuint skyboxVAO, skyboxVBO, quadVAO, quadVBO;
        GLuint skyboxTexture;

        GLuint irradianceMap;
        GLuint prefilterMap;
        GLuint brdfLUTTexture;

        int prefilterMaxMipLevels;
        static constexpr int IRRADIANCE_SIZE = 32;
        static constexpr int PREFILTER_SIZE = 128;
        static constexpr int BRDF_LUT_SIZE = 512;
        Path m_Path;
        GLuint loadCubemap(const std::vector<Path>& faces);
        void GenerateIBLMaps();
        void setupSkybox();
        void SetupQuad();
        void GenerateIrradianceMap();
        void GeneratePrefilterMap();
        void GenerateBRDFLUT();
        void SetupCaptureFBO(GLuint& captureFBO, GLuint& captureRBO, int size);
    };

}