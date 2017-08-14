#include "Convert.hpp"

#include <cassert>
#include <stdexcept>
#include <vector>
#include <sstream>

#include "resman/logger/Logger.hpp"

namespace resman {

template<typename T>
struct Alias_bgfx {
    std::vector<std::string> m_from;
    T m_to;
};
    
const std::vector<Alias_bgfx<std::string> > n_type_aliases = {
    {{"v", "vert", "vertex"}, "vertex"},
    {{"f", "frag", "fragment"}, "fragment"},
    {{"c", "comp", "compute"}, "compute"}
};

struct Pflags {
    std::string m_vert;
    std::string m_frag;
    std::string m_comp;
    
    const std::string& get_params(const std::string& type) const {
        if (type == "vertex") {
            return m_vert;
        } else if (type == "fragment") {
            return m_frag;
        } else if (type == "compute") {
            return m_comp;
        }
        assert(false && "Invalid type passed to Pflags");
    }
};

const std::string SHADER_INVALID = "__UNSUPPORTED__";

const std::vector<Alias_bgfx<Pflags> > n_platform_aliases = {
    {{"dx9"}, {
        "windows -p vs_3_0",
        "windows -p ps_3_0",
        SHADER_INVALID
    }},
    {{"dx11"}, {
        "windows -p vs_4_0",
        "windows -p ps_4_0",
        "windows -p cs_5_0"
    }},
    {{"pssl"}, {
        "orbis -p pssl",
        "orbis -p pssl",
        "orbis -p pssl"
    }},
    {{"metal"}, {
        "osx -p metal",
        "osx -p metal",
        "osx -p metal"
    }},
    {{"glsl"}, {
        "linux -p 120",
        "linux -p 120",
        "linux -p 430"
    }},
    {{"essl"}, {
        "android",
        "android",
        "android"
    }},
    {{"spirv"}, {
        "linux -p spirv",
        "linux -p spirv",
        "linux -p spirv"
    }}
};
    
const boost::filesystem::path n_std_shader_dir = "./bgfx/";

template<typename T>
const T& resolve(std::string input, 
        const std::vector<Alias_bgfx<T> >& mapping, const char* dbg_name) {
    for (const Alias_bgfx<T>& alias : mapping) {
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
    
    if (json_platform.asString() == "src") {
        boost::filesystem::copy(args.fromFile, args.outputFile);
        return;
    }
    
    /*
    Logger::log()->info("bgfx shader");
    Logger::log()->info(args.fromFile);
    Logger::log()->info(args.outputFile);
    Logger::log()->info(json_platform.asString());
    Logger::log()->info(json_type.asString());
    */
    
    const std::string& sc_type = 
            resolve(json_type.asString(), n_type_aliases, "shader type");
    const std::string& sc_platform = 
            resolve(json_platform.asString(), n_platform_aliases, "platform")
                    .get_params(sc_type);
    
    if (sc_platform == SHADER_INVALID) {
        std::stringstream sss;
        sss << "There is no shader matching: type="
            << sc_type << ", platform="
            << json_platform.asString();
        throw std::runtime_error(sss.str());
    }
    
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
    
    if (!boost::filesystem::exists(args.outputFile)
            || boost::filesystem::file_size(args.outputFile) == 0) {
        std::stringstream sss;
        sss << "shaderc failed to output file "
            << args.outputFile;
        throw std::runtime_error(sss.str());
    }
}


} // namespace resman

