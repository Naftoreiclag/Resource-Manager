#include "Convert.hpp"

#include <fstream>
#include <functional>
#include <iostream>

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "assimp/anim.h"

#include "StreamWrite.hpp"
    
void debugAiNode(const aiScene* scene, const aiNode* node, unsigned int depth) {

    for(int c = 0; c < depth; ++ c) {
        std::cout << "\t";
    }

    std::cout << node->mName.C_Str();

    // Matrix
    if(!node->mTransformation.IsIdentity()) {
        std::cout << " (transformed)";
    }
    
    unsigned int numMeshes = node->mNumMeshes;


    if(numMeshes > 0) {
        std::cout << " ";
        std::cout << numMeshes;
        std::cout << ":[";
        for(unsigned int i = 0; i < numMeshes; ++ i) {

            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            std::cout << mesh->mName.C_Str();

            // Nice commas
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

const aiNode* findNodeByLambda(const aiNode* node, std::function<bool(const aiNode*)> func) {
    if(func(node)) {
        return node;
    }
    for(unsigned int i = 0; i < node->mNumChildren; ++ i) {
        const aiNode* search = findNodeByLambda(node->mChildren[i], func);
        if(search) {
            return search;
        }
    }
    
    return nullptr;
}

const aiNode* findNodeByName(const aiNode* node, std::string name) {
    //return node->FindNode(aiString(name));
    
    aiString aString(name);
    return findNodeByLambda(node, [aString](const aiNode* node2) {
            return node2->mName == aString;
        });
}

uint32_t findNodeTreeSize(const aiNode* node) {
    uint32_t sum = 1;
    for(uint32_t i = 0; i < node->mNumChildren; ++ i) {
        sum += findNodeTreeSize(node->mChildren[i]);
    }
    return sum;
}

    
struct Vertex {
    // Position
    float x;
    float y;
    float z;

    // Color
    float r;
    float g;
    float b;
    float a;

    // Texture
    float u;
    float v;

    // Normal
    float nx;
    float ny;
    float nz;
    
    // Tangent
    float tx;
    float ty;
    float tz;
    
    // Bitangent
    float btx;
    float bty;
    float btz;

    Vertex()
    : x(0.f)
    , y(0.f)
    , z(0.f)
    , r(1.f)
    , g(1.f)
    , b(1.f)
    , a(1.f)
    , u(0.f)
    , v(0.f)
    , nx(1.f)
    , ny(0.f)
    , nz(0.f) {}
};

struct Triangle {
    uint32_t a;
    uint32_t b;
    uint32_t c;
};

struct Bone {
    uint32_t mIndex;
    uint32_t mParentIndex;
    bool mHasParent;
    
    std::string mName;
    std::vector<uint32_t> mChildren;
    
    Bone()
    : mIndex(0)
    , mParentIndex(0)
    , mHasParent(false) {
    }
};

typedef std::vector<Vertex> VertexBuffer;
typedef std::vector<Triangle> TriangleBuffer;
typedef std::vector<Bone> BoneBuffer;

struct Mesh {
    VertexBuffer mVertices;
    TriangleBuffer mTriangles;
    BoneBuffer mBones;

    bool mUseColor;
    bool mUseUV;
    bool mUseNormals;
    bool mUseLocations;
    bool mUseTangents;
    bool mUseBitangents;
    bool mUseBoneWeights;
    
    
};

enum SkinningTechnique {
    LINEAR_BLEND,
    DUAL_QUAT,
    IMPLICIT
};

SkinningTechnique stringToSkinningTechnique(std::string skinning) {
    if(skinning == "linear-blend") {
        return SkinningTechnique::LINEAR_BLEND;
    }
    else if(skinning == "dual-quaternion") {
        return SkinningTechnique::DUAL_QUAT;
    }
    else if(skinning == "implicit") {
        return SkinningTechnique::IMPLICIT;
    }
    
    return SkinningTechnique::LINEAR_BLEND;
}


float floatsy(float val) {
    return val < 0.0000005f ? 0.f : val;
}

void printThis(const aiMatrix4x4& aoffsetMatrix) {

    std::cout << floatsy(aoffsetMatrix.a1) << "\t| " << floatsy(aoffsetMatrix.a2) << "\t| " << floatsy(aoffsetMatrix.a3) << "\t| " << floatsy(aoffsetMatrix.a4) << "\t| " << std::endl;
    std::cout << floatsy(aoffsetMatrix.b1) << "\t| " << floatsy(aoffsetMatrix.b2) << "\t| " << floatsy(aoffsetMatrix.b3) << "\t| " << floatsy(aoffsetMatrix.b4) << "\t| " << std::endl;
    std::cout << floatsy(aoffsetMatrix.c1) << "\t| " << floatsy(aoffsetMatrix.c2) << "\t| " << floatsy(aoffsetMatrix.c3) << "\t| " << floatsy(aoffsetMatrix.c4) << "\t| " << std::endl;
    std::cout << floatsy(aoffsetMatrix.d1) << "\t| " << floatsy(aoffsetMatrix.d2) << "\t| " << floatsy(aoffsetMatrix.d3) << "\t| " << floatsy(aoffsetMatrix.d4) << "\t| " << std::endl;
}

uint32_t recursiveBuildBoneStructure(const aiNode* copyFrom, BoneBuffer& bones, uint32_t parent = 0, bool hasParent = false) {
    uint32_t boneIndex = bones.size();
    bones.push_back(Bone());
    Bone& bone = bones.back();
    
    bone.mIndex = boneIndex;
    bone.mHasParent = hasParent;
    bone.mParentIndex = parent;
    bone.mName = copyFrom->mName.C_Str();

    bone.mChildren.reserve(copyFrom->mNumChildren);
    for(uint32_t i = 0; i < copyFrom->mNumChildren; ++ i) {
        bone.mChildren.push_back(recursiveBuildBoneStructure(copyFrom->mChildren[i], bones, boneIndex, true));
    }
    
    return boneIndex;
}

void convertGeometry(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    
    Assimp::Importer assimp;
    
    bool paramMeshNameSpecified = false;
    std::string paramMeshName = "";
    bool paramPretransform = true;
    bool paramFlipWinding = false;
    
    bool paramLocationsRemove = false;
    
    bool paramNormalsRemove = false;
    
    bool paramUvsFlip = true;
    bool paramUvsRemove = false;
    
    bool paramTangentsGenerate = false;
    bool paramTangentsRemove = false;
    
    bool paramColorsRemove = false;
    
    bool paramBoneWeightsNormalize = false;
    SkinningTechnique paramBoneWeightsSkinningTechnique = SkinningTechnique::LINEAR_BLEND;
    
    bool paramLightprobesEnabled = false;
    std::string paramLightprobesMeshName = "";
    bool paramLightprobesBoneWeightsNormalize = false;
    SkinningTechnique  paramLightprobesBoneWeightSkinningTechnique = SkinningTechnique::LINEAR_BLEND;
    
    bool paramArmatureEnabled = false;
    std::string paramArmatureRootName = "";
    
    {
        {
            const Json::Value& jsonMeshName = params["mesh-name"];
            if(jsonMeshName.isString()) {
                paramMeshNameSpecified = true;
                paramMeshName = jsonMeshName.asString();
            }
        }
        
        {
            const Json::Value& jsonPretransform = params["pretransform"];
            if(jsonPretransform.isBool()) paramPretransform = jsonPretransform.asBool();
        }
        
        {
            const Json::Value& jsonLocations = params["locations"];
            if(!jsonLocations.isNull()) {
                const Json::Value& jsonRemove = jsonLocations["remove"];
                if(jsonRemove.isBool()) paramLocationsRemove = jsonRemove.asBool();
            }
        }
        
        {
            const Json::Value& jsonNormals = params["normals"];
            if(!jsonNormals.isNull()) {
                const Json::Value& jsonRemove = jsonNormals["remove"];
                if(jsonRemove.isBool()) paramNormalsRemove = jsonRemove.asBool();
            }
        }
        
        {
            const Json::Value& jsonUvs = params["uvs"];
            if(!jsonUvs.isNull()) {
                const Json::Value& jsonFlip = jsonUvs["flip"];
                if(jsonFlip.isBool()) paramUvsFlip = jsonFlip.asBool();
                
                const Json::Value& jsonRemove = jsonUvs["remove"];
                if(jsonRemove.isBool()) paramUvsRemove = jsonRemove.asBool();
            }
        }
        
        {
            const Json::Value& jsonTangents = params["tangents"];
            if(!jsonTangents.isNull()) {
                const Json::Value& jsonGenerate = jsonTangents["generate"];
                if(jsonGenerate.isBool()) paramTangentsGenerate = jsonGenerate.asBool();
                
                const Json::Value& jsonRemove = jsonTangents["remove"];
                if(jsonRemove.isBool()) paramTangentsRemove = jsonRemove.asBool();
            }
        }
        
        {
            const Json::Value& jsonColors = params["colors"];
            if(!jsonColors.isNull()) {
                const Json::Value& jsonRemove = jsonColors["remove"];
                if(jsonRemove.isBool()) paramColorsRemove = jsonRemove.asBool();
            }
        }
        
        {
            const Json::Value& jsonBones = params["bone-weights"];
            if(!jsonBones.isNull()) {
                const Json::Value& jsonNormalize = jsonBones["normalize"];
                if(jsonNormalize.isBool()) paramBoneWeightsNormalize = jsonNormalize.asBool();
                
                const Json::Value& jsonSkinning = jsonBones["skinning-technique"];
                if(jsonSkinning.isString()) {
                    paramBoneWeightsSkinningTechnique = stringToSkinningTechnique(jsonSkinning.asString());
                    paramLightprobesBoneWeightSkinningTechnique = paramBoneWeightsSkinningTechnique;
                }
            }
        }
        
        {
            const Json::Value& jsonLight = params["lightprobes"];
            if(!jsonLight.isNull()) {
                const Json::Value& jsonName = jsonLight["mesh-name"];
                if(jsonName.isString()) {
                    paramLightprobesEnabled = true;
                    paramLightprobesMeshName = jsonName.asString();
                }
                
                const Json::Value& jsonWeights = jsonLight["bone-weights"];
                if(!jsonWeights.isNull()) {
                    const Json::Value& jsonNormalize = jsonWeights["normalize"];
                    if(jsonNormalize.isBool()) paramLightprobesBoneWeightsNormalize = jsonNormalize.asBool();
                    
                    const Json::Value& jsonSkinning = jsonWeights["skinning-technique"];
                    if(jsonSkinning.isString()) paramLightprobesBoneWeightSkinningTechnique = stringToSkinningTechnique(jsonSkinning.asString());
                        
                }
            }
        }
        
        {
            const Json::Value& jsonArmature = params["armature"];
            if(!jsonArmature.isNull()) {
                const Json::Value& jsonRootName = jsonArmature["root-name"];
                if(jsonRootName.isString()) {
                    paramArmatureEnabled = true;
                    paramArmatureRootName = jsonRootName.asString();
                }
            }
        }
    }
    
    
    const aiScene* aScene;
    {
        unsigned int importFlags = aiProcess_Triangulate;
        
        if(paramPretransform) {
            importFlags |= aiProcess_PreTransformVertices;
            std::cout << "\tPretransforming vertices" << std::endl;
        }
        
        if(paramFlipWinding) {
            importFlags |= aiProcess_FlipUVs;
            std::cout << "\tFlipping triangle windings" << std::endl;
        }
        
        if(paramTangentsGenerate) {
            importFlags |= aiProcess_CalcTangentSpace;
            std::cout << "\tGenerating tangents and bitangents" << std::endl;
        }
        
        aScene = assimp.ReadFile(fromFile.string().c_str(), importFlags);
    }
    
    if(!aScene->HasMeshes()) {
        std::cout << "\tERROR: Imported scene has no meshes!" << std::endl;
        return;
    }

    // TODO: support for multiple color and uv channels

    const aiNode* rootNode = aScene->mRootNode;

    debugAiNode(aScene, rootNode, 1);

    Mesh output;
    
    const aiMesh* aMesh;
    const aiNode* aMeshNode;
    
    if(paramMeshNameSpecified) {
        aMeshNode = findNodeByName(rootNode, paramMeshName);
        if(!aMeshNode) {
            std::cout << "\tERROR: Could not find mesh node named " << paramMeshName << "!" << std::endl;
            return;
        }
        
        if(aMeshNode->mNumMeshes == 0) {
            std::cout << "\tERROR: Mesh node named " << paramMeshName << " has no meshes!" << std::endl;
            return;
        }
        
        aMesh = aScene->mMeshes[aMeshNode->mMeshes[0]];
    } else {
        aMesh = aScene->mMeshes[0];
        aMeshNode = nullptr;
    }

    output.mUseNormals = !paramNormalsRemove && aMesh->HasNormals();
    output.mUseLocations = !paramLocationsRemove && aMesh->HasPositions();
    output.mUseTangents = !paramTangentsRemove && aMesh->HasTangentsAndBitangents();
    output.mUseBitangents = !paramTangentsRemove && aMesh->HasTangentsAndBitangents();
    
    output.mUseColor = false;
    if(!paramColorsRemove) {
        for(uint32_t i = 0; i < aMesh->mNumVertices; ++ i) {
            if(aMesh->HasVertexColors(i)) {
                output.mUseColor = true;
                break;
            }
        }
    }
    
    output.mUseUV = false;
    if(!paramNormalsRemove) {
        for(uint32_t i = 0; i < aMesh->mNumVertices; ++ i) {
            if(aMesh->HasTextureCoords(i)) {
                output.mUseUV = true;
                break;
            }
        }
    }

    output.mVertices.reserve(aMesh->mNumVertices);
    for(uint32_t i = 0; i < aMesh->mNumVertices; ++ i) {

        // Assimp specification states that these arrays are all mNumVertices in size

        Vertex vertex;

        if(output.mUseLocations) {
            aiVector3D aPos = aMesh->mVertices[i];
            vertex.x = aPos.x;
            vertex.y = aPos.y;
            vertex.z = aPos.z;
        }
        if(output.mUseColor) {
            const aiColor4D& aColor = aMesh->mColors[0][i];
            vertex.r = aColor.r;
            vertex.g = aColor.g;
            vertex.b = aColor.b;
            vertex.a = aColor.a;
        }
        if(output.mUseNormals) {
            aiVector3D aNormal = aMesh->mNormals[i];
            vertex.nx = aNormal.x;
            vertex.ny = aNormal.y;
            vertex.nz = aNormal.z;
        }
        if(output.mUseUV) {
            const aiVector3D& aUV = aMesh->mTextureCoords[0][i];
            vertex.u = aUV.x;
            vertex.v = aUV.y;
        }
        if(output.mUseTangents) {
            aiVector3D aTangent = aMesh->mTangents[i];
            vertex.tx = aTangent.x;
            vertex.ty = aTangent.y;
            vertex.tz = aTangent.z;
        }
        if(output.mUseBitangents) {
            aiVector3D aBitangent = aMesh->mBitangents[i];
            vertex.btx = aBitangent.x;
            vertex.bty = aBitangent.y;
            vertex.btz = aBitangent.z;
        }

        output.mVertices.push_back(vertex);
    }

    output.mTriangles.reserve(aMesh->mNumFaces);
    for(uint32_t i = 0; i < aMesh->mNumFaces; ++ i) {
        const aiFace& aFace = aMesh->mFaces[i];

        Triangle triangle;
        triangle.a = aFace.mIndices[0];
        triangle.b = aFace.mIndices[1];
        triangle.c = aFace.mIndices[2];

        output.mTriangles.push_back(triangle);
    }
    
    if(paramArmatureEnabled) {
        const aiNode* aArmRoot = findNodeByName(aScene->mRootNode, paramArmatureRootName);
        
        if(aArmRoot) {
            uint32_t numBones = findNodeTreeSize(aArmRoot);
            
            if(numBones < 256) {
                std::cout << "Armature bone count: " << numBones << std::endl;
                
                output.mBones.reserve(numBones);
                recursiveBuildBoneStructure(aArmRoot, output.mBones);
            }
            else {
                std::cout << "\tERROR: Armature bone count (" << numBones << ") exceeds max (255)" << std::endl;
            }
        }
        else {
            std::cout << "\tERROR: Could not find armature root by name " << paramArmatureRootName << std::endl;
        }
    }

    std::cout << "\tVertices: " << output.mVertices.size() << std::endl;
    std::cout << "\tTriangles: " << output.mTriangles.size() << std::endl;

    {
        std::ofstream outputData(outputFile.string().c_str(), std::ios::out | std::ios::binary);

        writeBool(outputData, output.mUseLocations);
        writeBool(outputData, output.mUseColor);
        writeBool(outputData, output.mUseUV);
        writeBool(outputData, output.mUseNormals);
        writeBool(outputData, output.mUseTangents);
        writeBool(outputData, output.mUseBitangents);
        writeU32(outputData, output.mVertices.size());
        writeU32(outputData, output.mTriangles.size());
        for(VertexBuffer::iterator iter = output.mVertices.begin(); iter != output.mVertices.end(); ++ iter) {
            const Vertex& vertex = *iter;

            if(output.mUseLocations) {
                writeF32(outputData, vertex.x);
                writeF32(outputData, vertex.y);
                writeF32(outputData, vertex.z);
            }
            if(output.mUseColor) {
                writeF32(outputData, vertex.r);
                writeF32(outputData, vertex.g);
                writeF32(outputData, vertex.b);
                writeF32(outputData, vertex.a);
            }
            if(output.mUseUV) {
                writeF32(outputData, vertex.u);
                writeF32(outputData, vertex.v);
            }
            if(output.mUseNormals) {
                writeF32(outputData, vertex.nx);
                writeF32(outputData, vertex.ny);
                writeF32(outputData, vertex.nz);
            }
            if(output.mUseTangents) {
                writeF32(outputData, vertex.tx);
                writeF32(outputData, vertex.ty);
                writeF32(outputData, vertex.tz);
            }
            if(output.mUseBitangents) {
                writeF32(outputData, vertex.btx);
                writeF32(outputData, vertex.bty);
                writeF32(outputData, vertex.btz);
            }
        }
        for(TriangleBuffer::iterator iter = output.mTriangles.begin(); iter != output.mTriangles.end(); ++ iter) {
            const Triangle& triangle = *iter;

            writeU32(outputData, triangle.a);
            writeU32(outputData, triangle.b);
            writeU32(outputData, triangle.c);
        }

        outputData.close();
    }

    //boost::filesystem::copy_file(fromFile, outputFile);
}
