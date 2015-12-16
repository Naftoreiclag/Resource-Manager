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
        boost::filesystem::path mFile;
    };

    boost::filesystem::path mFile;
    boost::filesystem::path mDir;
    
    std::string mName;
    
    std::vector<Object> objects;
    
    inline void readProjectData() {
        Json::Value projectData;
        std::ifstream infile(mFile.c_str());
        infile >> projectData;
        infile.close();
        
        mName = projectData["name"].asString();
        
        std::cout << "Project Name: " << mName << std::endl;
        std::cout << std::endl;
    }
    
    inline void parseObjects() {
        std::cout << "Searching for objects..." << std::endl;
        std::vector<boost::filesystem::path> objectFiles;
        recursiveSearch(mDir, ".object", objectFiles);
        std::cout << std::endl;
        
        std::cout << "Found " << objectFiles.size() << " objects" << std::endl;
        std::cout << std::endl;
        
        for(std::vector<boost::filesystem::path>::iterator iter = objectFiles.begin(); iter != objectFiles.end(); ++ iter) {
            
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
