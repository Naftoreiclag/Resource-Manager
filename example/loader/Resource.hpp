#ifndef RESOURCE_HPP
#define RESOURCE_HPP

#include <stdint.h>

#include <boost/filesystem.hpp>

class Resource {
private:
    uint32_t mNumGrabs;
    uint32_t mFileSize;
    std::string mName;
    boost::filesystem::path mFile;
public:
    Resource();
    virtual ~Resource();
    
    void setFile(const boost::filesystem::path& file);
    const boost::filesystem::path& getFile();
    void setName(std::string name);
    const std::string& getName();
    void setSize(uint32_t size);
    const uint32_t& getSize();
    
    void grab();
    void drop();
    
    virtual bool load() = 0;
    virtual bool unload() = 0;
};

#endif // RESOURCE_HPP
