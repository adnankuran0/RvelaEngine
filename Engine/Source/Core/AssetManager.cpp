#include "rvelapch.h"
#include "AssetManager.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

bool LoadObjModel(const std::string& filepath, std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib,&shapes,&materials,&err, filepath.c_str())) {
        if (!warn.empty()) {
            std::cerr << "Warning: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "Error: " << err << std::endl;
        }
        return false;
    }

    std::unordered_map<Vertex, unsigned int, VertexHash> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            // Vertex pozisyonlarý
            vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
            vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
            vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

            // Normaller
            if (index.normal_index >= 0) {
                vertex.normal[0] = attrib.normals[3 * index.normal_index + 0];
                vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
                vertex.normal[2] = attrib.normals[3 * index.normal_index + 2];
            }

            // UV koordinatlarý
            if (index.texcoord_index >= 0) {
                vertex.texcoord[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.texcoord[1] = attrib.texcoords[2 * index.texcoord_index + 1];
            }

            // Benzersiz vertexleri kontrol et ve ekle
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<unsigned int>(vertices.size() / 8);
                vertices.insert(vertices.end(), { vertex.position[0], vertex.position[1], vertex.position[2],
                                                  vertex.normal[0], vertex.normal[1], vertex.normal[2],
                                                  vertex.texcoord[0], vertex.texcoord[1] });
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    return true;
}