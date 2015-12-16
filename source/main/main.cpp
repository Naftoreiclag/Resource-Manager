#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void process(std::string filename) {
    std::cout << filename << std::endl;
    std::ifstream infile(filename.c_str());
    std::string line;
    while(std::getline(infile, line)) {
        std::cout << line << std::endl;
    }
    infile.close();
}

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::cout << 
        "Compiles a resource project into a load-ready resource package.\n"
        "\n"
        "Usage: MACBETH [options] source [destination]\n"
        "\n"
        ;
        
        return 0;
    }
    
    process(argv[1]);
    
    
    std::cout << "Hello World!" << std::endl;
    return 0;
}
