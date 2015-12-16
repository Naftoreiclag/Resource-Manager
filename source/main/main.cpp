#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "json/json.h"

void process(std::string filename) {
    std::cout << "Processing: " << filename << std::endl;
    std::ifstream infile(filename.c_str());
    Json::Value root;
    infile >> root;
    infile.close();
    
    
    std::string projectName = root["name"].asString();
    std::cout << "Project Name: " << projectName << std::endl;
    
    /*
    Json::Value& objects = root["objects"];
    for(Json::ValueIterator iter = objects.begin(); iter != objects.end(); ++ iter) {
        Json::Value& objectData = *iter;
        
        std::string objectFilename = objectData.asString();
        
        std::cout << objectFilename << std::endl;
    }
    */
    
}

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
    
    process(argv[1]);
    
    
    std::cout << "Hello World!" << std::endl;
    return 0;
}
