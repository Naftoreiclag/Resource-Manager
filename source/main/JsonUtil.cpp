#include "JsonUtil.hpp"

#include <fstream>

Json::Value readJsonFile(std::string filename) {
    Json::Value retVal;
    
    std::ifstream fileStream(filename.c_str());
    std::stringstream ss;
    std::string line;
    while(std::getline(fileStream, line)) {
        bool isComment = false;
        for(std::string::iterator iter = line.begin(); iter != line.end(); ++ iter) {
            char c = *iter;
            if(c == ' ' || c == '\t') {
                continue;
            }
            if(c == '#') {
                isComment = true;
            } else {
                break;
            }
        }
        
        if(isComment) {
            ss << '\n';
        } else {
            ss << line;
        }
    }
    fileStream.close();
    
    ss >> retVal;
    
    return retVal;
}

void writeJsonFile(std::string filename, Json::Value& value, bool compact) {
    std::ofstream outputData(filename.c_str());
    
    if(compact) {
        Json::FastWriter fastWriter;
        outputData << fastWriter.write(value);
    } else {
        Json::StyledWriter styledWriter;
        outputData << styledWriter.write(value);
    }
    outputData.close();
}
