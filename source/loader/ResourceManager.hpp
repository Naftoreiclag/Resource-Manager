#ifndef RESOURCEMANAGER_HPP
#define RESOURCEMANAGER_HPP

#include <map>

#include <boost/filesystem.hpp>

#include "Resource.hpp"
#include "TextResource.hpp"
#include "MiscResource.hpp"

class ResourceManager {
private:
    std::map<std::string, TextResource*> mTexts;
    std::map<std::string, MiscResource*> mMiscs;
public:
    ResourceManager();
    ~ResourceManager();

    void mapAll(boost::filesystem::path data);
    
    TextResource* findText(std::string name);
};


#endif // RESOURCEMANAGER_HPP


