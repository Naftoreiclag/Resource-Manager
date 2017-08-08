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

namespace resman {

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


// Might not be perfect
// This is unbelievably slow. Use only for comparing json structures of limited size
bool equivalentJson(const Json::Value& json1, const Json::Value& json2) {
    if(json1.isNull()) return json2.isNull();
    if(json1.isBool()) return json2.isBool() && json1.asBool() == json2.asBool();
    if(json1.isUInt64() && json2.isUInt64()) return json1.asUInt64() == json2.asUInt64();
    if(json1.isInt64() && json2.isInt64()) return json1.asInt64() == json2.asInt64();
    if(json1.isUInt() && json2.isUInt()) return json1.asUInt() == json2.asUInt();
    if(json1.isInt() && json2.isInt()) return json1.asInt() == json2.asInt();
    if(json1.isDouble() && json2.isDouble()) return json1.asDouble() == json2.asDouble();
    if(json1.isNumeric()) return json2.isNumeric() && json1.asDouble() == json2.asDouble();
    if(json1.isString()) return json2.isString() && json1.asString() == json2.asString();
    
    if(json1.isArray()) {
        if(json2.isArray() && json1.size() == json2.size()) {
            std::vector<const Json::Value*> array1;
            std::vector<const Json::Value*> array2;
            
            for(const Json::Value& val1 : json1) array1.push_back(&val1);
            for(const Json::Value& val2 : json2) array2.push_back(&val2);
            
            for(const Json::Value* val1 : array1) {
                auto match = array2.begin();
                while(match != array2.end()) {
                    if(equivalentJson(*val1, **match)) {
                        break;
                    }
                    ++ match;
                }
                if(match == array2.end()) {
                    return false;
                } else {
                    array2.erase(match);
                }
            }
            return true;
        }
        return false;
    }
    
    if(json1.isObject()) {
        if(json2.isObject() && json1.size() == json2.size()) {
            for(Json::ValueConstIterator iter = json1.begin(); iter != json1.end(); ++ iter) {
                const Json::Value& key = iter.key();
                const Json::Value& value = *iter;
                
                if(!equivalentJson(value, json2[key.asString()])) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
    
    return false;
}

} // namespace resman
