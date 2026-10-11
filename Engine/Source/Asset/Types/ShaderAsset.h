#pragma once
#include "Asset/Asset.h"
#include "Asset/AssetUUID.h"
#include "Core/Ref.h"
#include "Renderer/Shader.h"
#include "Renderer/CustomShaderGenerator.h"
#include "Core/Log.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <unordered_set>
#include <chrono>

namespace rv {

class ShaderAsset;

inline std::unordered_set<ShaderAsset*>& GetActiveShaderAssets()
{
    static std::unordered_set<ShaderAsset*> s_Active;
    return s_Active;
}

class ShaderAsset : public Asset
{
public:
    explicit ShaderAsset(AssetUUID uuid) : Asset(uuid)
    {
        GetActiveShaderAssets().insert(this);
    }

    ~ShaderAsset() override
    {
        GetActiveShaderAssets().erase(this);
    }

    std::filesystem::path filePath;
    std::filesystem::file_time_type lastWriteTime{};
    std::string source;
    bool isLit = true;
    ShaderRenderModes renderModes;
    std::vector<ShaderUniformDef> uniforms;
    std::unordered_map<std::string, Ref<Sampler>> samplers;
    std::string compileError;
    bool compileSuccess = false;

    Ref<Shader> staticShader;
    Ref<Shader> skeletalShader;

    bool Compile()
    {
        std::string glslSource;
        std::string genError;
        if (!CustomShaderGenerator::GenerateGLSL(source, isLit, renderModes, uniforms, glslSource, genError))
        {
            compileSuccess = false;
            compileError = genError;
            LOG_ERROR("Shader generation failed: {}", compileError);
            return false;
        }

        staticShader = CreateRef<Shader>();
        if (!staticShader->InitFromSource("CustomShader_" + GetUUID().ToString(), glslSource, {}, ENGINE_PATH("Shaders\\pbr.glsl")))
        {
            compileSuccess = false;
            compileError = staticShader->GetLastCompileLog();
            LOG_ERROR("Static shader compile failed for {}: {}", GetUUID().ToString(), compileError);
            return false;
        }

        skeletalShader = CreateRef<Shader>();
        if (!skeletalShader->InitFromSource("CustomShader_Skeletal_" + GetUUID().ToString(), glslSource, { "SKELETAL" }, ENGINE_PATH("Shaders\\pbr.glsl")))
        {
            compileSuccess = false;
            compileError = skeletalShader->GetLastCompileLog();
            LOG_ERROR("Skeletal shader compile failed for {}: {}", GetUUID().ToString(), compileError);
            return false;
        }

        samplers.clear();
        for (const auto& uDef : uniforms)
        {
            if (uDef.hasCustomSampler)
            {
                samplers[uDef.name] = CreateRef<Sampler>(uDef.samplerDesc);
            }
        }

        compileSuccess = true;
        compileError.clear();
        return true;
    }

    bool ReloadAndCompile(const std::filesystem::path& path = {})
    {
        std::filesystem::path targetPath = path.empty() ? filePath : path;
        if (!targetPath.empty())
        {
            std::ifstream file(targetPath);
            if (file)
            {
                std::ostringstream ss;
                ss << file.rdbuf();
                source = ss.str();
                filePath = targetPath;
                std::error_code ec;
                lastWriteTime = std::filesystem::last_write_time(targetPath, ec);
            }
        }
        return Compile();
    }

    bool IsValid() const { return compileSuccess && static_cast<bool>(staticShader); }

    const ShaderUniformDef* FindUniform(const std::string& name) const
    {
        for (const auto& u : uniforms)
        {
            if (u.name == name)
                return &u;
        }
        return nullptr;
    }

    static void CheckHotReload()
    {
        using namespace std::chrono;
        static auto lastCheck = steady_clock::now();
        auto now = steady_clock::now();
        if (duration_cast<milliseconds>(now - lastCheck).count() < 500)
            return;
        lastCheck = now;

        for (ShaderAsset* asset : GetActiveShaderAssets())
        {
            if (!asset || asset->filePath.empty())
                continue;

            std::error_code ec;
            if (!std::filesystem::exists(asset->filePath, ec))
                continue;

            auto currentWriteTime = std::filesystem::last_write_time(asset->filePath, ec);
            if (!ec)
            {
                if (asset->lastWriteTime == std::filesystem::file_time_type{})
                {
                    asset->lastWriteTime = currentWriteTime;
                }
                else if (currentWriteTime != asset->lastWriteTime)
                {
                    asset->lastWriteTime = currentWriteTime;
                    asset->ReloadAndCompile();
                }
            }
        }
    }

private:
    friend class ShaderLoader;
};

}
