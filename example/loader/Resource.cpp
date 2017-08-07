#include "Resource.hpp"

#include <cassert>

Resource::Resource()
: mNumGrabs(0) { }
Resource::~Resource() { }

void Resource::setFile(const boost::filesystem::path& file) {
    assert(mFile == boost::filesystem::path());
    mFile = file;
}
const boost::filesystem::path& Resource::getFile() {
    return mFile;
}
void Resource::setName(std::string name) {
    mName = name;
}
const std::string& Resource::getName() {
    return mName;
}
void Resource::setSize(uint32_t size) {
    mFileSize = size;
}
const uint32_t& Resource::getSize() {
    return mFileSize;
}

void Resource::grab() {
    ++ mNumGrabs;
    
    if(mNumGrabs > 0) {
        this->load();
    }
}

void Resource::drop() {
    assert(mNumGrabs != 0);
    -- mNumGrabs;
    
    if(mNumGrabs == 0) {
        this->unload();
    }
}
