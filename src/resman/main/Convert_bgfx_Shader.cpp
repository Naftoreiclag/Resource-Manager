#include "Convert.hpp"

#include <stdexcept>
#include <vector>
#include <sstream>

#include "resman/logger/Logger.hpp"

namespace resman {

struct Alias_bgfx {
    std::vector<std::string> m_from;
    std::string m_to;
};
    
const std::vector<Alias_bgfx> n_type_aliases = {
    {{"v", "vert", "vertex"}, "v"},
    {{"f", "frag", "fragment"}, "f"}
};

    
const std::vector<Alias_bgfx> n_platform_aliases = {
    {{"android"}, "android"},
    {{"asm.js"}, "asm.js"},
    {{"ios"}, "ios"},
    {{"linux"}, "linux"},
    {{"nacl"}, "nacl"},
    {{"osx"}, "osx"},
    {{"windows"}, "windows"}
};
    
const boost::filesystem::path n_std_shader_dir = "./bgfx/";

std::string resolve(std::string input, 
        const std::vector<Alias_bgfx>& mapping, const char* dbg_name) {
    for (const Alias_bgfx& alias : mapping) {
        auto& from = alias.m_from;
        if (std::find(from.begin(), from.end(), input) != from.end()) {
            return alias.m_to;
        }
    }
    std::stringstream sss;
    sss << "Unknown "
        << dbg_name
        << ": \""
        << input
        << '"';
    throw std::runtime_error(sss.str());
}

void convert_bgfx_shader(const Convert_Args& args) {
    
    Json::Value json_platform = args.params["platform"];
    Json::Value json_type = args.params["type"];
    
    /*
    Logger::log()->info("bgfx shader");
    Logger::log()->info(args.fromFile);
    Logger::log()->info(args.outputFile);
    Logger::log()->info(json_platform.asString());
    Logger::log()->info(json_type.asString());
    */
    
    std::string sc_platform = 
            resolve(json_platform.asString(), n_platform_aliases, "platform");
    std::string sc_type = 
            resolve(json_type.asString(), n_type_aliases, "shader type");
    
    std::stringstream cmd;
    cmd << "shaderc"
        << " -f "
        << args.fromFile.string()
        << " -o "
        << args.outputFile.string()
        << " -i "
        << n_std_shader_dir.string()
        << " --platform "
        << sc_platform
        << " --type "
        << sc_type;
    std::system(cmd.str().c_str());
    
    if (!boost::filesystem::exists(args.outputFile)) {
        std::stringstream sss;
        sss << "shaderc failed to output file "
            << args.outputFile;
        throw std::runtime_error(sss.str());
    }
}


} // namespace resman

