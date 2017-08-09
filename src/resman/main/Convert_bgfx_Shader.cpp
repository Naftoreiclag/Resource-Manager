#include "Convert.hpp"

#include <stdexcept>

#include "resman/logger/Logger.hpp"

namespace resman {

void convert_bgfx_shader(const Convert_Args& args) {
    
    Json::Value json_type = args.params["type"];
    Json::Value json_include = args.params["include"];
    
    Logger::log()->info("bgfx shader");
    Logger::log()->info(args.fromFile);
    Logger::log()->info(args.outputFile);
    
    throw std::runtime_error("Not implemented");
}


} // namespace resman

