#include "Convert.hpp"

#include <fstream>
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

const aiNode* findFirstMeshNode(const aiNode* node) {
    if(node->mNumMeshes > 0) {
        return node;
    }
    
    for(unsigned int i = 0; i < node->mNumChildren; ++ i) {
        const aiNode* search = findFirstMeshNode(node->mChildren[i]);
        if(search) {
            return search;
        }
    }
    
    return 0;
}

namespace geo {

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
    
    // Tangent/Bitangent
    float tx;
    float ty;
    float tz;
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

typedef std::vector<Vertex> VertexBuffer;
typedef std::vector<Triangle> TriangleBuffer;

struct Mesh {
    VertexBuffer vertices;
    TriangleBuffer triangles;

    bool useColor;
    bool useUV;
    bool useNormals;
    bool usePositions;
    bool useTangents;
};

}
float floatsy(float val) {
    return val < 0.00005f ? 0.f : val;
}

void printThis(const aiMatrix4x4& aoffsetMatrix) {

    std::cout << floatsy(aoffsetMatrix.a1) << "\t| " << floatsy(aoffsetMatrix.a2) << "\t| " << floatsy(aoffsetMatrix.a3) << "\t| " << floatsy(aoffsetMatrix.a4) << "\t| " << std::endl;
    std::cout << floatsy(aoffsetMatrix.b1) << "\t| " << floatsy(aoffsetMatrix.b2) << "\t| " << floatsy(aoffsetMatrix.b3) << "\t| " << floatsy(aoffsetMatrix.b4) << "\t| " << std::endl;
    std::cout << floatsy(aoffsetMatrix.c1) << "\t| " << floatsy(aoffsetMatrix.c2) << "\t| " << floatsy(aoffsetMatrix.c3) << "\t| " << floatsy(aoffsetMatrix.c4) << "\t| " << std::endl;
    std::cout << floatsy(aoffsetMatrix.d1) << "\t| " << floatsy(aoffsetMatrix.d2) << "\t| " << floatsy(aoffsetMatrix.d3) << "\t| " << floatsy(aoffsetMatrix.d4) << "\t| " << std::endl;
}

void convertGeometry(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    
    
    
    Assimp::Importer assimp;
    const aiScene* scene;
    {
        unsigned int importFlags = aiProcess_Triangulate;
        
        const Json::Value& tangentsData = params["tangents"];
        
        if(!tangentsData.isNull()) {
            bool generate = tangentsData["generate"].asBool();
            
            if(generate) {
                importFlags |= aiProcess_CalcTangentSpace;
                std::cout << "\tTangents: generate" << std::endl;
            }
        }
        
        scene = assimp.ReadFile(fromFile.string().c_str(), importFlags);
    }

    // TODO: support for multiple color and uv channels

    const aiNode* rootNode = scene->mRootNode;

    debugAiNode(scene, rootNode, 1);

    geo::Mesh mesh;
    
    const aiNode* aNode = findFirstMeshNode(rootNode);

    const aiMesh* aMesh = scene->mMeshes[aNode->mMeshes[0]];

    mesh.useNormals = aMesh->HasNormals();
    mesh.usePositions = aMesh->HasPositions();
    mesh.useTangents = aMesh->HasTangentsAndBitangents();
    mesh.useColor = false;
    for(uint32_t i = 0; i < aMesh->mNumVertices; ++ i) {
        if(aMesh->HasVertexColors(i)) {
            mesh.useColor = true;
            break;
        }
    }
    mesh.useUV = false;
    for(uint32_t i = 0; i < aMesh->mNumVertices; ++ i) {
        if(aMesh->HasTextureCoords(i)) {
            mesh.useUV = true;
            break;
        }
    }

    mesh.vertices.reserve(aMesh->mNumVertices);
    for(uint32_t i = 0; i < aMesh->mNumVertices; ++ i) {

        // Assimp specification states that these arrays are all mNumVertices in size

        geo::Vertex vertex;

        if(mesh.usePositions) {
            aiVector3D aPos = aMesh->mVertices[i];
            aPos *= aNode->mTransformation;
            vertex.x = aPos.x;
            vertex.y = aPos.y;
            vertex.z = aPos.z;
        }
        if(mesh.useColor) {
            const aiColor4D& aColor = aMesh->mColors[0][i];
            vertex.r = aColor.r;
            vertex.g = aColor.g;
            vertex.b = aColor.b;
            vertex.a = aColor.a;
        }
        if(mesh.useNormals) {
            aiVector3D aNormal = aMesh->mNormals[i];
            aiMatrix4x4 transf = aNode->mTransformation;
            
            // Simulating w = 0 (aNormal is a direction not a position)
            transf.a4 = 0.f;
            transf.b4 = 0.f;
            transf.c4 = 0.f;
            transf.d4 = 0.f;
            
            aNormal *= transf;
            vertex.nx = aNormal.x;
            vertex.ny = aNormal.y;
            vertex.nz = aNormal.z;
        }
        if(mesh.useUV) {
            const aiVector3D& aUV = aMesh->mTextureCoords[0][i];
            vertex.u = aUV.x;
            vertex.v = aUV.y;
        }
        if(mesh.useTangents) {
            const aiVector3D& aTangent = aMesh->mTangents[i];
            vertex.tx = aTangent.x;
            vertex.ty = aTangent.y;
            vertex.tz = aTangent.z;
            
            const aiVector3D& aBitangent = aMesh->mBitangents[i];
            vertex.btx = aBitangent.x;
            vertex.bty = aBitangent.y;
            vertex.btz = aBitangent.z;
        }

        mesh.vertices.push_back(vertex);
    }

    mesh.triangles.reserve(aMesh->mNumFaces);
    for(uint32_t i = 0; i < aMesh->mNumFaces; ++ i) {
        const aiFace& aFace = aMesh->mFaces[i];

        geo::Triangle triangle;
        triangle.a = aFace.mIndices[0];
        triangle.b = aFace.mIndices[1];
        triangle.c = aFace.mIndices[2];

        mesh.triangles.push_back(triangle);
    }

    std::cout << "\tVertices: " << mesh.vertices.size() << std::endl;
    std::cout << "\tTriangles: " << mesh.triangles.size() << std::endl;

    {
        std::ofstream outputData(outputFile.string().c_str(), std::ios::out | std::ios::binary);

        writeBool(outputData, mesh.usePositions);
        writeBool(outputData, mesh.useColor);
        writeBool(outputData, mesh.useUV);
        writeBool(outputData, mesh.useNormals);
        writeBool(outputData, mesh.useTangents);
        writeU32(outputData, mesh.vertices.size());
        writeU32(outputData, mesh.triangles.size());
        for(geo::VertexBuffer::iterator iter = mesh.vertices.begin(); iter != mesh.vertices.end(); ++ iter) {
            const geo::Vertex& vertex = *iter;

            if(mesh.usePositions) {
                writeF32(outputData, vertex.x);
                writeF32(outputData, vertex.y);
                writeF32(outputData, vertex.z);
            }
            if(mesh.useColor) {
                writeF32(outputData, vertex.r);
                writeF32(outputData, vertex.g);
                writeF32(outputData, vertex.b);
                writeF32(outputData, vertex.a);
            }
            if(mesh.useUV) {
                writeF32(outputData, vertex.u);
                writeF32(outputData, vertex.v);
            }
            if(mesh.useNormals) {
                writeF32(outputData, vertex.nx);
                writeF32(outputData, vertex.ny);
                writeF32(outputData, vertex.nz);
            }
            if(mesh.useTangents) {
                writeF32(outputData, vertex.tx);
                writeF32(outputData, vertex.ty);
                writeF32(outputData, vertex.tz);
                writeF32(outputData, vertex.btx);
                writeF32(outputData, vertex.bty);
                writeF32(outputData, vertex.btz);
            }
        }
        for(geo::TriangleBuffer::iterator iter = mesh.triangles.begin(); iter != mesh.triangles.end(); ++ iter) {
            const geo::Triangle& triangle = *iter;

            writeU32(outputData, triangle.a);
            writeU32(outputData, triangle.b);
            writeU32(outputData, triangle.c);
        }

        outputData.close();
    }

    //boost::filesystem::copy_file(fromFile, outputFile);
}
