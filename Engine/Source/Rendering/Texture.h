#pragma once

namespace rv { 

enum TextureFormat : uint8_t
{
    Unknown = 0,
    RGBA8,
    RGB8,
    R8,
    BC1, //DXT1
    BC3  //DXT5
};

class Texture
{
public:
    Texture();
    ~Texture();
    Texture(const std::string& path);
    void Init();
    void Destroy() const;
    void Bind(unsigned int activeTexture) const;
    void Bind() const;
    void GenerateFromImage(const std::string& path);
    void GenerateFromMemory(const uint8_t* data, int width, int height, TextureFormat format = TextureFormat::RGBA8, bool srgb = false);

    static Texture Create();
    static void ToImage(int width, int height, const unsigned char* data,int nrChannels);
    void GenerateMipmaps();

    unsigned int GetID() const;
    inline unsigned char* GetTexture() const;
    inline int GetWidth() const;
    inline int GetHeight() const;
    inline int GetNrChannels() const;
    inline const std::string& GetPath() ;

private:
    std::string m_Path;
    unsigned int m_Texture{};
    unsigned char* m_Data{};
    int m_Width{}, m_Height{}, m_NrChannels{};
};

}
