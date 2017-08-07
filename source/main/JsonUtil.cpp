/*
 *  Copyright 2016-2017 James Fong
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
