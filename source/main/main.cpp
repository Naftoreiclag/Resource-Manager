#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "json/json.h"

enum ObjectType {
    IMAGE,
    MATERIAL,
    MESH,
    MODEL,
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    GEOMETRY_SHADER,
    TEXT,
    
    OTHER
};

std::string translateData(const ObjectType& otype, const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile) {
    switch(otype) {
        case MESH: {
            // TODO: something else
            
            boost::filesystem::copy_file(fromFile, outputFile);
            break;
        }
        default: {
            boost::filesystem::copy_file(fromFile, outputFile);
            break;
        }
    }
    
    return outputFile.filename().c_str();
}

std::string typeToString(const ObjectType& tpe) {
    switch(tpe) {
        case IMAGE: return "image";
        case MATERIAL: return "material";
        case MESH: return "mesh";
        case MODEL: return "model";
        case VERTEX_SHADER: return "vertex-shader";
        case FRAGMENT_SHADER: return "fragment-shader";
        case GEOMETRY_SHADER: return "geometry-shader";
        case TEXT: return "text";
        default: return "other";
    }
}

ObjectType stringToType(const std::string& str) {
    if(str == "image") {
        return IMAGE;
    } else 
    if(str == "material") {
        return MATERIAL;
    } else 
    if(str == "mesh") {
        return MESH;
    } else 
    if(str == "model") {
        return MODEL;
    } else 
    if(str == "vertex-shader") {
        return VERTEX_SHADER;
    } else 
    if(str == "fragment-shader") {
        return FRAGMENT_SHADER;
    } else 
    if(str == "geometry-shader") {
        return GEOMETRY_SHADER;
    } else 
    if(str == "text") {
        return TEXT;
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
            std::ifstream fileStream(mFile.c_str());
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
        if(boost::filesystem::exists(configFile)) {
            Json::Value configData;
            std::ifstream fileStream(configFile.c_str());
            fileStream >> configData;
            fileStream.close();
            
            outputDir = mDir / (configData["output"].asString());
            obfuscate = configData["obfuscate"].asBool();
            forceOverwriteOutput = configData["force-overwrite-output"].asBool();
            
            Json::Value& ignoreData = configData["ignore"];
            
            for(Json::Value::iterator iter = ignoreData.begin(); iter != ignoreData.end(); ++ iter) {
                Json::Value& ignore = *iter;
                
                ignoreDirs.push_back(mDir / (ignore.asString()));
            }
        }
        std::cout << "Configuration:" << std::endl;
        std::cout << "\toutput = " << outputDir << std::endl;
        std::cout << "\tobfuscate = " << (obfuscate ? "true" : "false") << std::endl;
        std::cout << std::endl;
        
        if(boost::filesystem::exists(outputDir)) {
            if(forceOverwriteOutput) {
                boost::filesystem::remove_all(outputDir);
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
                    boost::filesystem::remove_all(outputDir);
                }
                
                // Cancel
                else {
                    return false;
                }
            }
        }
        boost::filesystem::create_directories(outputDir);
        
        std::cout << "Searching for resources..." << std::endl;
        std::vector<boost::filesystem::path> objectFiles;
        recursiveSearch(mDir, ".resource", objectFiles, &ignoreDirs);
        recursiveSearch(mDir, ".resources", objectFiles, &ignoreDirs);
        std::cout << std::endl;
        
        std::cout << "Found " << objectFiles.size() << " resources" << std::endl;
        std::cout << std::endl;
        
        for(std::vector<boost::filesystem::path>::iterator objectFileIter = objectFiles.begin(); objectFileIter != objectFiles.end(); ++ objectFileIter) {
            boost::filesystem::path& objectFile = *objectFileIter;
            Json::Value objectData;
            std::ifstream fileStream(objectFile.c_str());
            fileStream >> objectData;
            fileStream.close();
            
            if(objectData.isArray()) {
                for(Json::ValueIterator valueIter = objectData.begin(); valueIter != objectData.end(); ++ valueIter) {
                    Json::Value& subData = *valueIter;
                    
                    if(subData.isObject()) {
                        parseObject(subData, objectFile);
                    }
                    else {
                        std::cout << "Warning! Resource defined in " << objectFile << " is not valid! (value = " << subData.toStyledString() << ")" << std::endl;
                    }
                }
            }
            else if(objectData.isObject()) {
                parseObject(objectData, objectFile);
            }
            else {
                std::cout << "Warning! Resource defined at " << objectFile << " is not valid!" << std::endl;
            }
        }
        std::cout << std::endl;
        
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
                        std::cout << "\tOffending files:" << std::endl;
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
        
        std::cout << "Translating data..." << std::endl;
        std::cout << std::endl;
        
        Json::Value outputPackageData;
        Json::Value& objectListData = outputPackageData["objects"];
        unsigned int seqName = 0;
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
                outputObjectFile /= object.mFile.filename();
            }
            
            std::string finalOutputName = translateData(object.mType, object.mFile, outputObjectFile);
            
            Json::Value& objectDef = objectListData[object.mName];
            objectDef["name"] = object.mName;
            objectDef["type"] = typeToString(object.mType);
            objectDef["file"] = finalOutputName;
            
            std::cout << object.mName << std::endl;
        }
        std::cout << std::endl;
        
        std::cout << "Exporting data.package... ";
        {
            std::ofstream finalOutputFile((outputDir / "data.package").c_str());
            finalOutputFile << outputPackageData;
            finalOutputFile.close();
        }
        std::cout << "Done!" << std::endl;
        std::cout << std::endl;
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::cout << "\n"
        "\nCompiles a resource project into a load-ready resource package."
        "\n"
        "\nUsage: MACBETH [options] source"
        "\n"
        "\nOptions:"
        "\n\t-o\tForce overwrite previous exports"
        "\n"
        "\n"
        "\n"
        "\n"
        ;
        
        return 0;
    }
    
    
    
    Project project;
    project.process(argv[1]);
    return 0;
}
