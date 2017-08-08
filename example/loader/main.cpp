#include <iostream>

#include <boost/filesystem.hpp>

#include "ResourceManager.hpp"
#include "TextResource.hpp"

int main(int argc, char* argv[]) {
    
    std::cout << "Test resource loader" << std::endl;
    
    ResourceManager resman;
    boost::filesystem::path data = "../../../example/output/data.package";
    resman.mapAll(data);
    
    TextResource* couplet = resman.findText("Witches.text");
    couplet->grab();
    std::cout << couplet->getString() << std::endl;
    couplet->drop();
    
    TextResource* greeting = resman.findText("HelloWorld.text");
    greeting->grab();
    std::cout << greeting->getString() << std::endl;
    greeting->drop();
    
    return 0;
}
