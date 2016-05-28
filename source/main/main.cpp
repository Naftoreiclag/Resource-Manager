#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "json/json.h"
#include "MurmurHash3.h"

#include "Convert.hpp"

bool equivalentJson(const Json::Value& val1, const Json::Value& val2) {
    return val1 == val2;
}

enum ObjectType {
    IMAGE,
    MATERIAL,
    MODEL,
    COMPUTE_SHADER,
    VERTEX_SHADER,
    TESS_CONTROL_SHADER,
    TESS_EVALUATION_SHADER,
    GEOMETRY_SHADER,
    FRAGMENT_SHADER,
    SHADER_PROGRAM,
    STRING,
    TEXTURE,
    GEOMETRY,
    FONT,

    
    OTHER
};

// If this returns true, then all files of this type will be re-converted.
// This is useful for debugging WIP converters.
bool isWorkInProgressType(const ObjectType& type) {
    return type == GEOMETRY;
}

void translateData(const ObjectType& otype, const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, uint32_t& filesize, const Json::Value& params, bool modifyFilename) {

    switch(otype) {
        case IMAGE: {
            convertImage(fromFile, outputFile, params, modifyFilename);
            break;
        }
        case GEOMETRY: {
            convertGeometry(fromFile, outputFile, params, modifyFilename);
            break;
        }
        case FONT: {
            convertFont(fromFile, outputFile, params, modifyFilename);
            break;
        }
        default: {
            convertMiscellaneous(fromFile, outputFile, params, modifyFilename);
            break;
        }
    }
    std::ifstream sizeTest(outputFile.string().c_str(), std::ios::binary | std::ios::ate);
    filesize = sizeTest.tellg();
}

std::string typeToString(const ObjectType& tpe) {
    switch(tpe) {
        case IMAGE: return "image";
        case MATERIAL: return "material";
        case MODEL: return "model";
        case COMPUTE_SHADER: return "compute-shader";
        case VERTEX_SHADER: return "vertex-shader";
        case TESS_CONTROL_SHADER: return "tess-control-shader";
        case TESS_EVALUATION_SHADER: return "tess-evaluation-shader";
        case GEOMETRY_SHADER: return "geometry-shader";
        case FRAGMENT_SHADER: return "fragment-shader";
        case SHADER_PROGRAM: return "shader-program";
        case STRING: return "string";
        case TEXTURE: return "texture";
        case GEOMETRY: return "geometry";
        case FONT: return "font";
        default: return "other";
    }
}

ObjectType stringToType(const std::string& str) {
    if(str == "image") {
        return IMAGE;
    } else if(str == "material") {
        return MATERIAL;
    } else if(str == "model") {
        return MODEL;
    } else if(str == "compute-shader") {
        return COMPUTE_SHADER;
    } else if(str == "vertex-shader") {
        return VERTEX_SHADER;
    } else if(str == "tess-control-shader") {
        return TESS_CONTROL_SHADER;
    } else if(str == "tess-evaluation-shader") {
        return TESS_EVALUATION_SHADER;
    } else if(str == "geometry-shader") {
        return GEOMETRY_SHADER;
    } else if(str == "fragment-shader") {
        return FRAGMENT_SHADER;
    } else if(str == "shader-program") {
        return SHADER_PROGRAM;
    } else if(str == "string") {
        return STRING;
    } else if(str == "texture") {
        return TEXTURE;
    } else if(str == "geometry") {
        return GEOMETRY;
    } else if(str == "font") {
        return FONT;
    }
    return OTHER;
}
        
class Project {
public:
    Project() { }
    ~Project() { }
    
    struct Object {
        std::string mName;
        ObjectType mType;
        boost::filesystem::path mFile;
        boost::filesystem::path mDebugOrigin;
        Json::Value mParams;

        // Needed only by intermediate stuff
        uint32_t mOriginalSize;
        uint32_t mOriginalHash;
        bool mSkipTranslate = false;

        //
        boost::filesystem::path mOutputFile;
        uint32_t mOutputSize;

    };

    boost::filesystem::path mFile;
    boost::filesystem::path mDir;
    
    std::string mName;
    
    std::vector<Object> objects;
    
    void parseObject(const Json::Value& objectData, const boost::filesystem::path& objectFile) {
        Object object;
        object.mDebugOrigin = objectFile;
        object.mName = objectData["name"].asString();
        object.mType = stringToType(objectData["type"].asString());
        object.mParams = objectData["parameters"];
        if(object.mType == OTHER) {
            std::cout << "Warning! Unknown resource type " << objectData["type"].asString() << " found in resource " << object.mName << " found at " << object.mDebugOrigin << std::endl;
        }
        
        boost::filesystem::path newPath = objectFile;
        object.mFile = newPath.remove_filename() / objectData["file"].asString();
        
        objects.push_back(object);
        
        std::cout << "Resource: name = " << object.mName << std::endl;
        std::cout << "\ttype = " << typeToString(object.mType) << std::endl;
        std::cout << "\tfile = " << object.mFile << std::endl;
    }
    
    void recursiveSearch(const boost::filesystem::path& romeo, 
        const std::string& extension, 
        std::vector<boost::filesystem::path>& results, 
        std::vector<boost::filesystem::path>* ignore = 0) {
            
        if(!boost::filesystem::exists(romeo)) {
            return;
        }
        
        boost::filesystem::directory_iterator endIter;
        for(boost::filesystem::directory_iterator iter(romeo); iter != endIter; ++ iter) {
            boost::filesystem::path juliet = *iter;
            if(boost::filesystem::is_directory(juliet)) {
                bool search = true;
                if(ignore) {
                    for(std::vector<boost::filesystem::path>::iterator iter = ignore->begin(); iter != ignore->end(); ++ iter) {
                        if(boost::filesystem::equivalent(juliet, *iter)) {
                            search = false;
                            break;
                        }
                    }
                }
                if(search) {
                    recursiveSearch(juliet, extension, results, ignore);
                }
            }
            else {
                if(juliet.has_filename() && juliet.extension() == extension) {
                    results.push_back(juliet);
                    std::cout << "\t" << juliet.c_str() << std::endl;
                }
            }
        }
    }
    
    
    
    bool process(std::string filename) {
        std::cout << "Processing: " << filename << std::endl;
        mFile = filename;
        if(!boost::filesystem::exists(mFile)) {
            std::cout << "Project file does not exist!" << std::endl;
            return false;
        }
        mDir = mFile.parent_path();
        
        {
            Json::Value projectData;
            std::ifstream fileStream(mFile.string().c_str());
            fileStream >> projectData;
            fileStream.close();
            
            mName = projectData["name"].asString();
        }
        
        std::cout << "Project Name:" << std::endl;
        std::cout << "\t" << mName << std::endl;
        std::cout << std::endl;
        
        boost::filesystem::path configFile = mDir / "compile.config";
        bool obfuscate = false;
        std::vector<boost::filesystem::path> ignoreDirs;
        boost::filesystem::path outputDir = mDir / "__output__";
        bool forceOverwriteOutput = false;
        boost::filesystem::path intermediateDir;
        bool useIntermediate = false;
        if(boost::filesystem::exists(configFile)) {
            Json::Value configData;
            std::ifstream fileStream(configFile.string().c_str());
            fileStream >> configData;
            fileStream.close();
            
            outputDir = mDir / (configData["output"].asString());
            obfuscate = configData["obfuscate"].asBool();
            forceOverwriteOutput = configData["force-overwrite-output"].asBool();

            if(!configData["intermediate"].isNull()) {
                useIntermediate = true;
                intermediateDir = mDir / (configData["intermediate"].asString());
            }
            
            Json::Value& ignoreData = configData["ignore"];
            
            for(Json::Value::iterator iter = ignoreData.begin(); iter != ignoreData.end(); ++ iter) {
                Json::Value& ignore = *iter;
                
                ignoreDirs.push_back(mDir / (ignore.asString()));
            }
        }
        std::cout << "Configuration:" << std::endl;
        std::cout << "\toutput = " << outputDir << std::endl;
        if(useIntermediate) {
            std::cout << "\tintermediate = " << intermediateDir << std::endl;
        } else {
            std::cout << "\tuse intermediate = false" << std::endl;
        }
        std::cout << "\tobfuscate = " << (obfuscate ? "true" : "false") << std::endl;
        std::cout << std::endl;
        
        if(boost::filesystem::exists(outputDir)) {
            if(forceOverwriteOutput) {
                //boost::filesystem::remove_all(outputDir);
            }
            else {
                std::cout << "Warning! Output directory " << outputDir << " already exists!" << std::endl;
                bool decided = false;
                bool decision;
                while(!decided) {
                    std::cout << "Overwrite? (y/n) ";
                    std::string input;
                    std::cin >> input;
                    
                    char a = *input.begin();
                    if(a == 'y') {
                        decided = true;
                        decision = true;
                    }
                    else if(a == 'n') {
                        decided = true;
                        decision = false;
                    }
                }
                
                // Overwrite
                if(decision) {
                    //boost::filesystem::remove_all(outputDir);
                }
                
                // Cancel
                else {
                    return false;
                }
            }

            boost::filesystem::directory_iterator directoryEnd;
            for(boost::filesystem::directory_iterator dirIter(outputDir); dirIter != directoryEnd; ++ dirIter) {
                boost::filesystem::remove_all(*dirIter);
            }
        }
        boost::filesystem::create_directories(outputDir);
        
        std::cout << "Searching for resources..." << std::endl;
        std::vector<boost::filesystem::path> objectFiles;
        recursiveSearch(mDir, ".resource", objectFiles, &ignoreDirs);
        recursiveSearch(mDir, ".resources", objectFiles, &ignoreDirs);
        std::cout << std::endl;
        
        std::cout << "Found " << objectFiles.size() << " resource declaration files." << std::endl;
        std::cout << std::endl;
        
        for(std::vector<boost::filesystem::path>::iterator objectFileIter = objectFiles.begin(); objectFileIter != objectFiles.end(); ++ objectFileIter) {
            boost::filesystem::path& objectFile = *objectFileIter;
            Json::Value objectData;
            std::ifstream fileStream(objectFile.string().c_str());
            fileStream >> objectData;
            fileStream.close();
            
            if(objectData.isArray()) {
                for(Json::ValueIterator valueIter = objectData.begin(); valueIter != objectData.end(); ++ valueIter) {
                    Json::Value& subData = *valueIter;
                    
                    if(subData.isObject()) {
                        parseObject(subData, objectFile);
                    }
                    else {
                        std::cout << "Warning! Resource declared in " << objectFile << " is not valid! (value = " << subData.toStyledString() << ")" << std::endl;
                    }
                }
            }
            else if(objectData.isObject()) {
                parseObject(objectData, objectFile);
            }
            else {
                std::cout << "Warning! Resource declared at " << objectFile << " is not valid!" << std::endl;
            }
        }
        std::cout << std::endl;

        // Locate errors
        {
            typedef std::vector<boost::filesystem::path> PathList;
            typedef std::pair<std::string, PathList> NameObjectListPair;
            typedef std::vector<NameObjectListPair> ObjectMap;
            ObjectMap objectMap;
            
            bool foundErrors = false;
            for(std::vector<Object>::iterator iter = objects.begin(); iter != objects.end(); ++ iter) {
                Object& exam = *iter;
                
                PathList* pathList = 0;
                for(ObjectMap::iterator look = objectMap.begin(); look != objectMap.end(); ++ look) {
                    NameObjectListPair& pair = *look;
                    
                    // Oh no!
                    if(pair.first == exam.mName) {
                        pathList = &pair.second;
                        break;
                    }
                }
                
                if(pathList) {
                    foundErrors = true;
                    pathList->push_back(exam.mDebugOrigin);
                } else {
                    NameObjectListPair nolp;
                    nolp.first = exam.mName;
                    nolp.second.push_back(exam.mDebugOrigin);
                    objectMap.push_back(nolp);
                }
            }
            
            if(foundErrors) {
                for(ObjectMap::iterator look = objectMap.begin(); look != objectMap.end(); ++ look) {
                    NameObjectListPair& pair = *look;
                    
                    if(pair.second.size() > 1) {
                        std::cout << "Fatal! Detected naming conflict for resource \"" << pair.first << "\"" << std::endl;
                        std::cout << "\tOffending declaration files:" << std::endl;
                        for(PathList::iterator egg = pair.second.begin(); egg != pair.second.end(); ++ egg) {
                            std::cout << "\t" << (*egg) << std::endl;
                        }
                    }
                }
                std::cout << "Could not compile!" << std::endl;
                std::cout << std::endl;
                return false;
            }
            else {
                std::cout << "No naming conflicts detected!" << std::endl;
            }
            
        }
        std::cout << std::endl;

        // Do some important pre-calculations
        {
            uint32_t seqName = 0;
            for(std::vector<Object>::iterator iter = objects.begin(); iter != objects.end(); ++ iter) {
                Object& object = *iter;

                boost::filesystem::path outputObjectFile = outputDir;
                if(obfuscate) {
                    std::stringstream ss;
                    ss << (seqName ++);
                    ss << ".r";
                    outputObjectFile /= ss.str();
                }
                else {
                    std::stringstream ss;
                    ss << (seqName ++);
                    ss << "_";
                    ss << object.mFile.filename().string();
                    outputObjectFile /= ss.str();
                }

                object.mOutputFile = outputObjectFile;
            }
        }

        if(useIntermediate) {
            if(!boost::filesystem::exists(intermediateDir)) {
                boost::filesystem::create_directories(intermediateDir);
            }
            std::cout << "Calculating hashes..." << std::endl;
            for(std::vector<Object>::iterator iter = objects.begin(); iter != objects.end(); ++ iter) {

                Object& object = *iter;

                std::ifstream reader(object.mFile.string().c_str(), std::ios::binary | std::ios::ate);
                object.mOriginalSize = reader.tellg();

                char* totalData = new char[object.mOriginalSize];
                reader.seekg(0, std::ios::beg);
                reader.read(totalData, object.mOriginalSize);
                reader.close();

                MurmurHash3_x86_32(totalData, object.mOriginalSize, 1337, &(object.mOriginalHash));

                delete[] totalData;

                std::cout << "\t" << object.mOriginalHash << std::endl;
            }
            std::cout << std::endl;

            std::cout << "Checking for pre-compiled data..." << std::endl;

            boost::filesystem::path intermediateFile = intermediateDir / "intermediate.data";

            if(!boost::filesystem::exists(intermediateFile)) {
                std::cout << "None exists!" << std::endl;
            }
            else {
                Json::Value previousCompilation;
                {
                    std::ifstream reader(intermediateFile.string().c_str());
                    reader >> previousCompilation;
                    reader.close();
                }

                const Json::Value& metadataList = previousCompilation["metadata"];
                for(std::vector<Object>::iterator iter = objects.begin(); iter != objects.end(); ++ iter) {

                    /*
                     * If a pre-compiled file exists with exactly the same:
                     *  Hash
                     *  Configuration:
                     *      Type
                     *      Parameters
                     *
                     * Then simply copy the data into the output folder rather than translate it again.
                     *      (Therefore remove that object from the list of objects to translate.)
                     */

                    Object& object = *iter;

                    for(Json::Value::const_iterator meta = metadataList.begin(); meta != metadataList.end(); ++ meta) {
                        const Json::Value& metadata = *meta;

                        uint32_t checkHash = (uint32_t) (metadata["hash"].asInt64());
                        ObjectType checkType = stringToType(metadata["type"].asString());
                        const Json::Value& checkParams = metadata["params"];

                        if(checkHash == object.mOriginalHash && checkType == object.mType && equivalentJson(checkParams, object.mParams) && !isWorkInProgressType(object.mType)) {
                            std::cout << "\tCopy: " << object.mName << std::endl;
                            object.mSkipTranslate = true;

                            boost::filesystem::copy(intermediateDir / (metadata["file"].asString()), object.mOutputFile);
                            std::ifstream sizeTest(object.mOutputFile.string().c_str(), std::ios::binary | std::ios::ate);
                            object.mOutputSize = sizeTest.tellg();

                            break;
                        }
                    }
                }
            }
            std::cout << std::endl;
        }
        
        std::cout << "Translating data..." << std::endl;
        std::cout << std::endl;
        {
            boost::filesystem::path intermediateFile = intermediateDir / "intermediate.data";
            Json::Value intermediateData;
            uint32_t metadataIndex = 0;
            if(useIntermediate) {
                if(boost::filesystem::exists(intermediateFile)) {
                    std::ifstream reader(intermediateFile.string().c_str());
                    reader >> intermediateData;
                    reader.close();
                    metadataIndex = intermediateData["metadata"].size();
                }
            }

            uint64_t totalSize = 0;
            uint32_t numUpdates = 0;

            Json::Value outputPackageData;
            Json::Value& objectListData = outputPackageData["resources"];
            uint32_t jsonListIndex = 0;

            for(std::vector<Object>::iterator iter = objects.begin(); iter != objects.end(); ++ iter) {
                Object& object = *iter;

                std::cout << object.mName << " [" << typeToString(object.mType) << "]" << std::endl;

                if(!object.mSkipTranslate || isWorkInProgressType(object.mType)) {
                    std::cout << "\tTranslating..." << std::endl;
                    translateData(object.mType, object.mFile, object.mOutputFile, object.mOutputSize, object.mParams, !obfuscate);
                    ++ numUpdates;

                    if(useIntermediate) {
                        Json::Value& objectMetadata = intermediateData["metadata"][metadataIndex];

                        objectMetadata["hash"] = (Json::Int64) (object.mOriginalHash);
                        objectMetadata["type"] = typeToString(object.mType);
                        objectMetadata["params"] = object.mParams;

                        std::stringstream ss;
                        ss << object.mName;
                        ss << typeToString(object.mType);
                        ss << "-";
                        ss << ((uint32_t) (object.mOriginalHash));
                        ss << ".i";
                        std::string intermFilename = ss.str();

                        objectMetadata["file"] = intermFilename;

                        // copy file
                        boost::filesystem::path copyTo = intermediateDir / intermFilename;
                        if(boost::filesystem::exists(copyTo)) {
                            boost::filesystem::remove(copyTo);
                        }
                        boost::filesystem::copy(object.mOutputFile, copyTo);

                        ++ metadataIndex;
                    }
                }
                //
                Json::Value& objectDef = objectListData[jsonListIndex];
                objectDef["name"] = object.mName;
                objectDef["type"] = typeToString(object.mType);
                objectDef["file"] = object.mOutputFile.filename().string().c_str();
                objectDef["size"] = object.mOutputSize;

                totalSize += object.mOutputSize;

                ++ jsonListIndex;
            }
            std::cout << std::endl;
            std::cout << numUpdates << " file(s) updated" << std::endl;
            std::cout << std::endl;

            if(useIntermediate) {
                std::cout << "Exporting intermediate.data... ";
                {
                    std::ofstream intermOutput(intermediateFile.string().c_str());
                    intermOutput << intermediateData;
                    intermOutput.close();
                }
                std::cout << "Done!" << std::endl;
                std::cout << std::endl;
            }

            std::cout << "Exporting data.package... ";

            Json::Value& metricsData = outputPackageData["metrics"];
            metricsData["totalSize"] = (Json::UInt64) totalSize;
            {
                std::ofstream finalOutputFile((outputDir / "data.package").string().c_str());
                finalOutputFile << outputPackageData;
                finalOutputFile.close();
            }
            std::cout << "Done!" << std::endl;
            std::cout << std::endl;
        }
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::cout << "\n"
        "\nCompiles a resource project into a load-ready resource package."
        "\n"
        "\nUsage: MACBETH source"
        "\n"
        ;
        
        // ../../../example2/project/core.project
        
        return 0;
    }
    
    Project project;
    project.process(argv[1]);
    return 0;
}
