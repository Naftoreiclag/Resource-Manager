#include <iostream>

#include <boost/filesystem.hpp>

#include "ResourceManager.hpp"

int main(int argc, char* argv[]) {
    
    boost::filesystem::path data;
    ResourceManager::getSingleton().loadAll(data);
    
    std::cout << "Test resource loader" << std::endl;
    return 0;
}
