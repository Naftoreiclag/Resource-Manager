/*
 *  Copyright 2015-2017 James Fong
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

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

namespace resman {

void debugAiNode(const aiScene* scene, const aiNode* node, uint32_t depth, bool lastChild) {

    char vert = '|';
    char con = '+';
    char conlast = '-';
    
    std::cout << "\t";
    for(uint32_t c = 0; c < depth; ++ c) {
        if(c == depth - 1) {
            if(lastChild) {
                std::cout << conlast;
            } else {
                std::cout << con;
            }
        } else {
            std::cout << vert;
        }
        std::cout << ' ';
    }

    std::cout << "[" << node->mName.C_Str() << "]";

    // Matrix
    if(!node->mTransformation.IsIdentity()) {
        std::cout << "(T)"; // "T" = "transformed"
    }
    
    uint32_t numMeshes = node->mNumMeshes;
    if(numMeshes > 0) {
        std::cout << ":";
        std::cout << numMeshes;
        std::cout << "{";
        for(uint32_t i = 0; i < numMeshes; ++ i) {

            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            std::cout << "[" << mesh->mName.C_Str() << "]";

            // Nice commas
            if(i != numMeshes - 1) {
                std::cout << ", ";
            }
        }
        std::cout << "}";

    }
    
    std::cout << std::endl;

    uint32_t numChildren = node->mNumChildren;
    for(uint32_t i = 0; i < numChildren; ++ i) {
        debugAiNode(scene, node->mChildren[i], depth + 1, i == numChildren - 1);
    }
}

void debugAssimp(const Assimp::Importer& assimp, const aiScene* aScene) {
    
    std::cout << "\tESTR: " << assimp.GetErrorString() << std::endl;
    
    std::cout << "\tCOUNT:" << std::endl;
    std::cout << "\t\tANIMATIONS: " << aScene->mNumAnimations << std::endl;
    std::cout << "\t\tCAMERAS: " << aScene->mNumCameras << std::endl;
    std::cout << "\t\tLIGHTS: " << aScene->mNumLights << std::endl;
    std::cout << "\t\tMATERIALS: " << aScene->mNumMaterials << std::endl;
    std::cout << "\t\tMESHES: " << aScene->mNumMaterials << std::endl;
    std::cout << "\t\tTEXTURES: " << aScene->mNumTextures << std::endl;
    
    for(uint32_t i = 0; i < aScene->mNumMaterials; ++ i) {
        const aiMaterial* aMaterial = aScene->mMaterials[i];
    }
    for(uint32_t i = 0; i < aScene->mNumMeshes; ++ i) {
        const aiMesh* aMesh = aScene->mMeshes[i];
        std::cout << "\tMESH: [" << aMesh->mName.C_Str() << "]" << std::endl;
        std::cout << "\t\tVERTS: " << aMesh->mNumVertices << std::endl;
        std::cout << "\t\tTRIS: " << aMesh->mNumFaces << std::endl;
    }
    
    debugAiNode(aScene, aScene->mRootNode, 0, false);

}

const aiNode* findNodeByLambda(const aiNode* node, std::function<bool(const aiNode*)> func) {
    if(func(node)) {
        return node;
    }
    for(uint32_t i = 0; i < node->mNumChildren; ++ i) {
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


enum SkinningTechnique {
    LINEAR_BLEND = 0,
    DUAL_QUAT = 1,
    IMPLICIT = 2
};

struct BoneWeight {
    uint8_t id;
    float weight;
};
typedef std::vector<BoneWeight> BoneWeightBuffer;
    
struct Vertex {
    uint32_t mIndex;
    
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
    
    // Bone weights
    BoneWeightBuffer mBoneWeights;

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

struct Lightprobe {
    // Position
    float x;
    float y;
    float z;
    
    //
    bool mUseBoneWeights;
    
    // Bone weights
    BoneWeightBuffer mBoneWeights;
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
typedef std::vector<Lightprobe> LightprobeBuffer;
typedef std::vector<Bone> BoneBuffer;

struct Mesh {
    VertexBuffer mVertices;
    TriangleBuffer mTriangles;
    LightprobeBuffer mLightprobes;
    BoneBuffer mBones;
    
    SkinningTechnique mVertexSkinning;
    SkinningTechnique mLightprobeSkinning;

    bool mUseColor;
    bool mUseUV;
    bool mUseNormals;
    bool mUseLocations;
    bool mUseTangents;
    bool mUseBitangents;
    bool mUseBoneWeights;
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

uint8_t skinningTechniqueToByte(SkinningTechnique st) {
    return (uint8_t) st;
}

inline void writeBoneBuffer(std::ofstream& outputData, const BoneWeightBuffer& boneBuffer) {
    assert(boneBuffer.size() <= 4);
    uint16_t missingBones = 4 - boneBuffer.size();
    for(const BoneWeight bw : boneBuffer) writeU8(outputData, bw.id);
    for(uint16_t i = 0; i < missingBones; ++ i) writeU8(outputData, 0);
    for(const BoneWeight bw : boneBuffer) writeF32(outputData, bw.weight);
    for(uint16_t i = 0; i < missingBones; ++ i) writeF32(outputData, 0.f);
}

void outputMesh(Mesh& output, const boost::filesystem::path& outputFile) {
    std::ofstream outputData(outputFile.string().c_str(), std::ios::out | std::ios::binary);
    
    // Exist flags
    bool useVertices = true;
    bool useTriangles = true;
    bool useBones = output.mBones.size() > 0;
    bool useLightprobes = output.mLightprobes.size() > 0;
    writeU8(outputData,
        useVertices |
        useTriangles << 1 |
        useBones << 2 |
        useLightprobes << 3
    );
    
    if(useVertices) {
        // Per-vertex data bit flags
        writeU8(outputData,
            output.mUseLocations |
            output.mUseColor << 1 |
            output.mUseUV << 2 |
            output.mUseNormals << 3 |
            output.mUseTangents << 4 |
            output.mUseBitangents << 5 |
            output.mUseBoneWeights << 6
        );
        writeU8(outputData, skinningTechniqueToByte(output.mVertexSkinning));
        writeU32(outputData, output.mVertices.size());
        for(Vertex& vertex : output.mVertices) {
            // Every vertex has a fixed size in bytes, allowing for "random access" of vertices if necessary
            
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
            if(output.mUseBoneWeights) {
                writeBoneBuffer(outputData, vertex.mBoneWeights);
            }
        }
    }
    
    if(useTriangles) {
        writeU32(outputData, output.mTriangles.size());
        if(output.mVertices.size() <= 1 << 8) {
            for(Triangle& triangle : output.mTriangles) {
                writeU8(outputData, (uint8_t) triangle.a);
                writeU8(outputData, (uint8_t) triangle.b);
                writeU8(outputData, (uint8_t) triangle.c);
            }
        }
        else if(output.mVertices.size() <= 1 << 16) {
            for(Triangle& triangle : output.mTriangles) {
                writeU16(outputData, (uint16_t) triangle.a);
                writeU16(outputData, (uint16_t) triangle.b);
                writeU16(outputData, (uint16_t) triangle.c);
            }
        }
        else {
            for(Triangle& triangle : output.mTriangles) {
                writeU32(outputData, (uint32_t) triangle.a);
                writeU32(outputData, (uint32_t) triangle.b);
                writeU32(outputData, (uint32_t) triangle.c);
            }
        }
    }
    
    if(useBones) {
        assert(output.mBones.size() <= 256);
        writeU8(outputData, output.mBones.size() - 1);
        for(Bone& bone : output.mBones) {
            // Bones are not fixed in size:
            // - Varying length of bone names
            // - Varying length of child array
            
            writeString(outputData, bone.mName);
            writeBool(outputData, bone.mHasParent);
            if(bone.mHasParent) writeU8(outputData, bone.mParentIndex);
            writeU8(outputData, bone.mChildren.size());
            for(uint32_t child : bone.mChildren) writeU8(outputData, child);
        }
    }
    
    if(useLightprobes) {
        assert(output.mLightprobes.size() <= 256);
        writeU8(outputData, skinningTechniqueToByte(output.mLightprobeSkinning));
        writeU8(outputData, output.mLightprobes.size() - 1);
        for(Lightprobe& lightprobe : output.mLightprobes) {
            // Lightprobes are also not fixed in size
            writeF32(outputData, lightprobe.x);
            writeF32(outputData, lightprobe.y);
            writeF32(outputData, lightprobe.z);
            writeBool(outputData, lightprobe.mUseBoneWeights);
            if(lightprobe.mUseBoneWeights) {
                writeBoneBuffer(outputData, lightprobe.mBoneWeights);
            }
        }
    }

    outputData.close();
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
    // Importer must be kept alive.
    // Importer destroys imported data upon destruction.
    Assimp::Importer assimp;
    
    bool paramMeshNameSpecified = false;
    std::string paramMeshName = "";
    bool paramPretransform = false;
    bool paramFlipWinding = false;
    
    bool paramLocationsRemove = false;
    
    bool paramNormalsGenerate = false;
    bool paramNormalsRemove = false;
    
    bool paramUvsFlip = true;
    bool paramUvsGenerate = false;
    bool paramUvsRemove = false;
    
    bool paramTangentsGenerate = false;
    bool paramTangentsRemove = false;
    
    bool paramColorsRemove = false;
    
    bool paramBoneWeightsNormalize = false;
    double paramBoneWeightsAbsMinWeight = 0.0;
    SkinningTechnique paramBoneWeightsSkinningTechnique = SkinningTechnique::LINEAR_BLEND;
    bool paramBoneWeightsRemove = false;
    
    bool paramLightprobesEnabled = false;
    std::string paramLightprobesMeshName = "";
    bool paramLightprobesBoneWeightsNormalize = false;
    double paramLightprobesBoneWeightsAbsMinWeight = 0.0;
    SkinningTechnique  paramLightprobesBoneWeightSkinningTechnique = SkinningTechnique::LINEAR_BLEND;
    
    bool paramArmatureEnabled = false;
    std::string paramArmatureRootName = "";
    
    // Read from json configuration
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
                const Json::Value& jsonGenerate = jsonNormals["generate"];
                if(jsonGenerate.isBool()) paramNormalsGenerate = jsonGenerate.asBool();
                
                const Json::Value& jsonRemove = jsonNormals["remove"];
                if(jsonRemove.isBool()) paramNormalsRemove = jsonRemove.asBool();
            }
        }
        
        {
            const Json::Value& jsonUvs = params["uvs"];
            if(!jsonUvs.isNull()) {
                const Json::Value& jsonFlip = jsonUvs["flip"];
                if(jsonFlip.isBool()) paramUvsFlip = jsonFlip.asBool();
                
                const Json::Value& jsonGenerate = jsonUvs["generate"];
                if(jsonGenerate.isBool()) paramUvsGenerate = jsonGenerate.asBool();
                
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
                
                const Json::Value& jsonAbsMin = jsonBones["absolute-minimum-weight"];
                if(jsonAbsMin.isNumeric()) paramBoneWeightsAbsMinWeight = jsonAbsMin.asDouble();
                
                const Json::Value& jsonSkinning = jsonBones["skinning-technique"];
                if(jsonSkinning.isString()) {
                    paramBoneWeightsSkinningTechnique = stringToSkinningTechnique(jsonSkinning.asString());
                    paramLightprobesBoneWeightSkinningTechnique = paramBoneWeightsSkinningTechnique;
                }
                
                const Json::Value& jsonRemove = jsonBones["remove"];
                if(jsonRemove.isBool()) paramBoneWeightsRemove = jsonRemove.asBool();
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
    
    uint32_t importFlags = 0;
    //importFlags |= aiProcess_CalcTangentSpace;
    importFlags |= aiProcess_Debone;
    //importFlags |= aiProcess_FindDegenerates;
    //importFlags |= aiProcess_FindInstances;
    importFlags |= aiProcess_FindInvalidData;
    // importFlags |= aiProcess_FixInfacingNormals;
    // importFlags |= aiProcess_FlipUVs;
    // importFlags |= aiProcess_FlipWindingOrder;
    // importFlags |= aiProcess_GenNormals;
    //importFlags |= aiProcess_GenSmoothNormals;
    //importFlags |= aiProcess_GenUVCoords;
    importFlags |= aiProcess_ImproveCacheLocality;
    importFlags |= aiProcess_JoinIdenticalVertices; // Otherwise every face will have its own vertices!!
    //importFlags |= aiProcess_LimitBoneWeights;
    // importFlags |= aiProcess_MakeLeftHanded;
    // importFlags |= aiProcess_OptimizeGraph;
    importFlags |= aiProcess_OptimizeMeshes;
    // importFlags |= aiProcess_PreTransformVertices;
    // importFlags |= aiProcess_RemoveComponent;
    //importFlags |= aiProcess_RemoveRedundantMaterials;
    //importFlags |= aiProcess_SortByPType;
    // importFlags |= aiProcess_SplitByBoneCount;
    //importFlags |= aiProcess_SplitLargeMeshes;
    // importFlags |= aiProcess_TransformUVCoords;
    importFlags |= aiProcess_Triangulate;
    importFlags |= aiProcess_ValidateDataStructure;
    if(paramNormalsGenerate) {
        importFlags |= aiProcess_GenSmoothNormals;
        std::cout << "\tGenerating normals" << std::endl;
    }
    
    if(paramUvsGenerate) {
        importFlags |= aiProcess_GenUVCoords;
        std::cout << "\tGenerating UVs" << std::endl;
    }
    
    if(paramTangentsGenerate) {
        importFlags |= aiProcess_CalcTangentSpace;
        std::cout << "\tGenerating tangents and bitangents" << std::endl;
    }
    
    if(paramPretransform) {
        importFlags |= aiProcess_PreTransformVertices;
        std::cout << "\tPretransforming vertices" << std::endl;
        std::cout << "\tWARNING: Pretransform breaks animation data!" << std::endl;
    }
    
    if(paramUvsFlip) {
        importFlags |= aiProcess_FlipUVs;
        std::cout << "\tFlipping UVs" << std::endl;
    }
    
    if(paramFlipWinding) {
        importFlags |= aiProcess_FlipWindingOrder;
        std::cout << "\tFlipping triangle windings" << std::endl;
    }
    
    std::cout << "\tArmature " << (paramArmatureEnabled ? "enabled" : "disabled") << std::endl;
    std::cout << "\tLightprobes " << (paramLightprobesEnabled ? "enabled" : "disabled") << std::endl;
     
    // Import scene
    const aiScene* aScene = assimp.ReadFile(fromFile.string().c_str(), importFlags);

    // Display debug information
    debugAssimp(assimp, aScene);
    
    if(!aScene->HasMeshes()) {
        std::cout << "\tERROR: Imported scene has no meshes!" << std::endl;
        return;
    }

    // TODO: support for multiple color and uv channels

    const aiNode* aRootNode = aScene->mRootNode;

    Mesh output;
    
    const aiMesh* aMesh;
    const aiNode* aMeshNode;
    
    if(paramMeshNameSpecified) {
        aMeshNode = findNodeByName(aRootNode, paramMeshName);
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
    output.mUseBoneWeights = !paramBoneWeightsRemove && aMesh->HasBones();
    
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
    if(!paramUvsRemove) {
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
        
        vertex.mIndex = i;

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
    output.mVertexSkinning = paramBoneWeightsSkinningTechnique;

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
            
            if(numBones > 256) {
                std::cout << "\tERROR: Armature bone count (" << numBones << ") exceeds max (256)" << std::endl;
            }
            else {
                std::cout << "Armature bone count: " << numBones << std::endl;
                
                output.mBones.reserve(numBones);
                recursiveBuildBoneStructure(aArmRoot, output.mBones);
            }
        }
        else {
            std::cout << "\tERROR: Could not find armature root by name " << paramArmatureRootName << std::endl;
        }
    }

    if(output.mUseBoneWeights) {
        for(uint32_t iBone = 0; iBone < aMesh->mNumBones; ++ iBone) {
            const aiBone* aBone = aMesh->mBones[iBone];
            // Each aiBone is really more of a pointer into the bone array by name..?
            
            // Find the index of the appropriate bone
            std::string boneName = aBone->mName.C_Str();
            uint8_t boneIndex;
            for(boneIndex = 0; boneIndex < output.mBones.size(); ++ boneIndex) {
                const Bone& bone = output.mBones.at(boneIndex);
                if(bone.mName == boneName) {
                    break;
                }
            }
            if(boneIndex == output.mBones.size()) {
                std::cout << "\tERROR: Could not find bone named: " << boneName << std::endl;
            } else {
                for(uint32_t iWeight = 0; iWeight < aBone->mNumWeights; ++ iWeight) {
                    const aiVertexWeight& aWeight = aBone->mWeights[iWeight];
                    Vertex& vert = output.mVertices.at(aWeight.mVertexId);
                    BoneWeight weight;
                    weight.id = boneIndex;
                    weight.weight = aWeight.mWeight;
                    vert.mBoneWeights.push_back(weight);
                }
            }
        }
        
        // Sort bone weights by influence (descending order)
        // Normalize if requested
        for(Vertex& vertex : output.mVertices) {
            
            BoneWeightBuffer& boneWeights = vertex.mBoneWeights;
            
            // Skip procesing on any vertex not affected by bones
            if(boneWeights.size() > 0) {
                // Exclude any weights that are less than the given minimum
                if(paramBoneWeightsAbsMinWeight > 0.0) {
                    boneWeights.erase(std::remove_if(
                        boneWeights.begin(), boneWeights.end(),
                        [paramBoneWeightsAbsMinWeight](const BoneWeight& bw) -> bool {
                            if(bw.weight < 0.0) {
                                return -bw.weight < paramBoneWeightsAbsMinWeight;
                            } else {
                                return bw.weight < paramBoneWeightsAbsMinWeight;
                            }
                        }
                    ), boneWeights.end());
                }
                
                // Sort bone weights
                std::sort(boneWeights.begin(), boneWeights.end(),
                    [](const BoneWeight& a, const BoneWeight& b) {
                        // Sort by descending order of weight, unless the weights are equal.
                        // In that case sort by ascending id's
                        return (a.weight == b.weight) ? (a.id < b.id) : (a.weight > b.weight);
                    }
                );
                    
                // Make sure that maximum influence count does not exceed maximum
                if(boneWeights.size() > 4) {
                    std::cout << "\tWARNING: Vertex " << vertex.mIndex << " has " << boneWeights.size() << " > 4 bones" << std::endl;
                    
                    // Restore original total weight (unless normalization is specified, in which case the total weight is 1)
                    if(!paramBoneWeightsNormalize) {
                        double lostWeight = 0.0;
                        uint32_t index = 0;
                        for(BoneWeight bw : boneWeights) {
                            if(index >= 4) lostWeight += bw.weight;
                            ++ index;
                        }
                        boneWeights.resize(4);
                        lostWeight /= 4.0;
                        for(BoneWeight& bw : boneWeights) {
                            bw.weight += lostWeight;
                        }
                    } else {
                        boneWeights.resize(4);
                    }
                }
                
                // Normalization if requested (sets the total weight for the bones to be one)
                if(paramBoneWeightsNormalize) {
                    double totalWeight = 0.0;
                    for(BoneWeight bw : boneWeights) {
                        totalWeight += bw.weight;
                    }
                    
                    // Avoid division by zero
                    if(totalWeight != 0.0) {
                        for(BoneWeight bw : boneWeights) {
                            bw.weight /= totalWeight;
                        }
                    } else {
                        // Just set the first bone to have maximum influence
                        // Note that bones with exactly zero bone influences are already excluded
                        boneWeights.at(0).weight = 1.0;
                    }
                }
            }
        }
    }
    
    output.mLightprobeSkinning = paramLightprobesBoneWeightSkinningTechnique;
    
    std::cout << "\tVertices: " << output.mVertices.size() << std::endl;
    std::cout << "\tTriangles: " << output.mTriangles.size() << std::endl;
    std::cout << "\tBones: " << output.mBones.size() << std::endl;
    std::cout << "\tLightprobes: " << output.mLightprobes.size() << std::endl;
    
    outputMesh(output, outputFile);
    
    // Debug
    outputMesh(output, fromFile.parent_path() / "debug_geometry");
}

} // namespace resman

