#include "Convert.hpp"

#include <iostream>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "assimp/anim.h"

void debugAiNode(const aiScene* scene, const aiNode* node, unsigned int depth) {

    for(int c = 0; c < depth; ++ c) {
        std::cout << "  ";
    }

    std::cout << node->mName.C_Str();

    unsigned int numMeshes = node->mNumMeshes;

    if(numMeshes > 0) {
        std::cout << " ";
        std::cout << numMeshes;
        std::cout << ":[";
        for(unsigned int i = 0; i < numMeshes; ++ i) {

            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            std::cout << mesh->mName.C_Str();

            /*
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            if(material->mNumProperties > 0) {
                std::cout << ":[";
                for(unsigned int j = 0; j < material->mNumProperties; ++ j) {
                    aiMaterialProperty* property = material->mProperties[j];
                    aiPropertyTypeInfo* info = property->mType;
                    std::cout << property->mKey.C_Str() << "=" << property->mData << std::endl;
                }
                std::cout << "]";
            }
            */

            if(i != numMeshes - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "]";

    }
    std::cout << std::endl;

    unsigned int numChildren = node->mNumChildren;

    for(unsigned int i = 0; i < numChildren; ++ i) {
        debugAiNode(scene, node->mChildren[i], depth + 1);
    }

}

void convertGeometry(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    Assimp::Importer assimp;
    const aiScene* scene = assimp.ReadFile(fromFile.c_str(), aiProcessPreset_TargetRealtime_Fast);

    const aiNode* rootNode = scene->mRootNode;

    debugAiNode(scene, rootNode, 0);

    boost::filesystem::copy_file(fromFile, outputFile);
}
