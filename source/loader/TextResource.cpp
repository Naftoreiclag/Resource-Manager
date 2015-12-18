#include "TextResource.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

TextResource::TextResource()
: mLoaded(false) {
}

TextResource::~TextResource() {
}

bool TextResource::load() {
    if(mLoaded) {
        return true;
    }
    
    
    std::ifstream loader(this->getFile().c_str());
    std::stringstream ss;
    ss << loader.rdbuf();
    loader.close();
    
    mData = ss.str();
    
    return true;
}

bool TextResource::unload() {
    return true;
}

const std::string& TextResource::getString() {
    return mData;
}
