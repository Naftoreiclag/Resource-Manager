#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "json/json.h"

class Project {
public:
    Project() { }
    ~Project() { }
    
    struct Object {
        enum Type {
            IMAGE = 1,
            MATERIAL = 2,
            MESH = 3,
            MODEL = 4,
            
            UNKNOWN = 0
        };
        
        static Type stringToType(const std::string& str) {
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
            }
            return UNKNOWN;
        }
        
        std::string mName;
        Type mType;
        boost::filesystem::path mFile;
    };

    boost::filesystem::path mFile;
    boost::filesystem::path mDir;
    
    std::string mName;
    
    std::vector<Object> objects;
    
    inline void readProjectData() {
        Json::Value projectData;
        std::ifstream fileStream(mFile.c_str());
        fileStream >> projectData;
        fileStream.close();
        
        mName = projectData["name"].asString();
        
        std::cout << "Project Name: " << mName << std::endl;
        std::cout << std::endl;
    }
    
    void parseObject(const Json::Value& objectData, const boost::filesystem::path& objectFile) {
        Object object;
        object.mName = objectData["name"].asString();
        object.mType = Object::stringToType(objectData["type"].asString());
        boost::filesystem::path newPath = objectFile;
        object.mFile = newPath.remove_filename() / objectData["file"].asString();
        
        std::cout << "Object parsed: (name = " << object.mName << "; type = " << object.mType << "; file = " << object.mFile << ";)" << std::endl;
    }
    
    inline void parseObjects() {
        std::cout << "Searching for objects..." << std::endl;
        std::vector<boost::filesystem::path> objectFiles;
        recursiveSearch(mDir, ".object", objectFiles);
        std::cout << std::endl;
        
        std::cout << "Found " << objectFiles.size() << " objects" << std::endl;
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
                        std::cout << "Warning! Object defined in " << objectFile << " is not valid! (value = " << subData.toStyledString() << ")" << std::endl;
                    }
                }
            }
            else if(objectData.isObject()) {
                parseObject(objectData, objectFile);
            }
            else {
                std::cout << "Warning! Object defined at " << objectFile << " is not valid!" << std::endl;
            }
        }
    }
    
    void recursiveSearch(const boost::filesystem::path& romeo, const std::string& extension, std::vector<boost::filesystem::path>& results) {
        if(!boost::filesystem::exists(romeo)) {
            return;
        }
        
        boost::filesystem::directory_iterator endIter;
        for(boost::filesystem::directory_iterator iter(romeo); iter != endIter; ++ iter) {
            boost::filesystem::path juliet = *iter;
            if(boost::filesystem::is_directory(juliet)) {
                recursiveSearch(juliet, extension, results);
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
        
        readProjectData();
        parseObjects();
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::cout << 
        "Compiles a resource project into a load-ready resource package.\n"
        "\n"
        "Usage: MACBETH [options] source [destination]\n"
        "\n"
        ;
        
        return 0;
    }
    
    Project project;
    project.process(argv[1]);
    
    
    std::cout << "Hello World!" << std::endl;
    return 0;
}
