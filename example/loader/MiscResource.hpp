#ifndef MiscRESOURCE_HPP
#define MiscRESOURCE_HPP

#include "Resource.hpp"

class MiscResource : public Resource {
public:
    MiscResource();
    virtual ~MiscResource();
    
    bool load();
    bool unload();
};

#endif // MiscRESOURCE_HPP
