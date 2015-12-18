#ifndef TEXTRESOURCE_HPP
#define TEXTRESOURCE_HPP

#include "Resource.hpp"

class TextResource : public Resource {
private:
    std::string mData;
    bool mLoaded;
public:
    TextResource();
    virtual ~TextResource();
    
    
    bool load();
    bool unload();
    
    const std::string& getString();

};

#endif // TEXTRESOURCE_HPP
