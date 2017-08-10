#include "Expand.hpp"

#include <array>

#include "resman/logger/Logger.hpp"

namespace resman {
    
const std::array<const char*, 7> n_all_shader_platforms = {
    "android",
    "asm.js",
    "ios",
    "linux",
    "nacl",
    "osx",
    "windows"
};

std::vector<Expansion> expand_bgfx_shader(const Object& obj) {
    std::vector<Expansion> retval;
    
    Json::Value json_platform = obj.m_params["platform"];
    
    if (!json_platform.isNull() && json_platform.asString() != "all") {
        return retval;
    }
    
    for (const char* shader_platform : n_all_shader_platforms) {
        Expansion expand;
        expand.m_obj = obj;
        expand.m_obj.m_params["platform"] = shader_platform;
        expand.m_subtype = shader_platform;
        retval.emplace_back(std::move(expand));
    }
    
    return retval;
}


} // namespace resman
