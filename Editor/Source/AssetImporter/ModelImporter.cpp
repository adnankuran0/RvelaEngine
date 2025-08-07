#include "ModelImporter.h"

bool ModelImporter::Import(const std::filesystem::path& path)
{
	
	//bool resultTextureImport = m_TextureImporter.Import(path);
	//bool resultMaterialImport = m_MaterialImporter.Import(path);
	bool resultMeshImport = m_MeshImporter.Import(path);
	return resultMeshImport;
}