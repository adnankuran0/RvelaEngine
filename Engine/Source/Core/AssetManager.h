#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

// Vertex verisi için struct
struct Vertex {
    float position[3];
    float normal[3];
    float texcoord[2];

    bool operator==(const Vertex& other) const {
        return std::equal(std::begin(position), std::end(position), std::begin(other.position)) &&
            std::equal(std::begin(normal), std::end(normal), std::begin(other.normal)) &&
            std::equal(std::begin(texcoord), std::end(texcoord), std::begin(other.texcoord));
    }
};

// Hashing için özel fonksiyon
struct VertexHash {
    size_t operator()(const Vertex& vertex) const {
        size_t pos_hash = std::hash<float>{}(vertex.position[0]) ^ std::hash<float>{}(vertex.position[1]) ^ std::hash<float>{}(vertex.position[2]);
        size_t norm_hash = std::hash<float>{}(vertex.normal[0]) ^ std::hash<float>{}(vertex.normal[1]) ^ std::hash<float>{}(vertex.normal[2]);
        size_t tex_hash = std::hash<float>{}(vertex.texcoord[0]) ^ std::hash<float>{}(vertex.texcoord[1]);
        return pos_hash ^ norm_hash ^ tex_hash;
    }
};

// OBJ yükleyip vertices ve indices vektörlerini dolduran fonksiyon
bool LoadObjModel(const std::string& filepath, std::vector<float>& vertices, std::vector<unsigned int>& indices);